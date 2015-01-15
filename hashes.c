#define USE "find ... -print0 | verity_hashes [-t as-of-time_t] 'db connection string'"

#include <arpa/inet.h>
#include <libpq-fe.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <fgetsnull.h>

#include "err.h"

int main(int argc, char ** argv){
	char path[PATH_MAX+1];
	PGconn *db;
	PGresult *result;
	uint64_t t=htobe64(time(NULL));
	int c;
	while ((c=getopt(argc,argv,"t:"))!=-1){ switch(c){
		case 't': t=htobe64(strtoll(optarg,NULL,10)); break;
		default: fputs(USE,stderr); exit(EXIT_FAILURE); }}
	if (!(argc-optind)) { fputs(USE,stderr); exit(EXIT_FAILURE); }
	db=PQconnectdb(argv[optind]);
	if	(PQstatus(db)!=CONNECTION_OK)
		{	fputs(PQerrorMessage(db),stderr);
			exit(EXIT_FAILURE); }

	while(!feof(stdin)){
		if (!fgetsnull(path,PATH_MAX+1,stdin)) break;
		result=PQexecParams(db,
			"select content from inodes "\
				"join paths on paths.device=inodes.device and paths.inode=inodes.inode and paths.ctime=inodes.ctime "\
				"where mode/4096=8 and path=$1 "\
				"and xtime= (select max(xtime) from paths as alias where alias.path=paths.path and xtime<=$2)",
			2,NULL,
			(char const * const []){ path, (char const * const)&t},
			(int const []){0, sizeof(uint64_t) },
			(int const []){0,1},0);
		SQLCHECK(db,result,PGRES_TUPLES_OK,err1);
		c=PQntuples(result);
		if (!c) { PQclear(result); continue; }
		if(PQntuples(result)!=1) {fputs("sanity check failed - multiple hashes for path",stderr); goto err1; }
		printf("%s%s%c",PQgetvalue(result,0,0),path,'\0');
		PQclear(result); }

	if (ferror(stdin)) { perror("stdin"); goto l0; }
	PQfinish(db);
	return 0;
	err1:	PQclear(result);
	l0:	PQfinish(db);
		exit(EXIT_FAILURE); }

/*IN GOD WE TRVST.*/
