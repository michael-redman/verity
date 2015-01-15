#define USE "verity_list [-c] [-t as-of-time_t] 'db connection string'"

#define _GNU_SOURCE

#include <libpq-fe.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "err.h"

#define SQL_DECLARE	"declare paths_cursor cursor for select paths.path "

#ifndef CORRELATED_SUBQUERY

#define JOIN0	", mode, content "
#define JOIN1	"from paths "\
		"join (select path, max(xtime) as max_xtime from paths where xtime<=%ld group by path) as max_xtimes on max_xtimes.path=paths.path and max_xtimes.max_xtime=paths.xtime "
#define JOIN2	"join inodes on inodes.device=paths.device and inodes.inode=paths.inode and inodes.ctime=paths.ctime "
#define JOIN3	"where paths.device is not null"

#else

#define SQ0	"(select mode from inodes where inodes.device=paths.device and inodes.inode=paths.inode and inodes.ctime=paths.ctime) as mode, "\
		"(select content from inodes where inodes.device=paths.device and inodes.inode=paths.inode and inodes.ctime=paths.ctime) as content "
#define SQ1	"from paths where "\
		"device is not null "\
		"and not exists (select * from paths as alias where alias.path=paths.path and alias.xtime>paths.xtime and alias.xtime<=%ld)"

#endif

int main(int argc, char ** argv){
	PGconn *db;
	PGresult *result;
	char *sql, *sql_f, flag=0;
	time_t t=time(NULL);
	int c;
	while ((c=getopt(argc,argv,"ct:"))!=-1){ switch(c){
		case 'c': flag=1; break;
		case 't': t=strtoll(optarg,NULL,10); break;
		default: fputs(USE,stderr); exit(EXIT_FAILURE); }}
	if (!(argc-optind)) { fputs(USE,stderr); exit(EXIT_FAILURE); }
	db=PQconnectdb(argv[optind]);
	if(PQstatus(db)!=CONNECTION_OK){
		fputs(PQerrorMessage(db),stderr);
		exit(EXIT_FAILURE); }
	result=PQexec(db,"begin");
	SQLCHECK(db,result,PGRES_COMMAND_OK,err0);
	#ifndef CORRELATED_SUBQUERY
	if	(flag)
		sql_f=SQL_DECLARE JOIN0 JOIN1 JOIN2 JOIN3;
		else sql_f=SQL_DECLARE JOIN1 JOIN3;
	#else
	if	(flag)
		sql_f=SQL_DECLARE SQ0 SQ1;
		else sql_f=SQL_DECLARE SQ1;
	#endif
	if	(asprintf(&sql,sql_f,t)==-1)
		{ fputs("asprintf failed",stderr); goto err0; }
	PQclear(result);
	result=PQexec(db,sql);
	free(sql);
	SQLCHECK(db,result,PGRES_COMMAND_OK,err0);
	PQclear(result);
	while(1){
		result=PQexec(db,"fetch paths_cursor");
		SQLCHECK(db,result,PGRES_TUPLES_OK,err0);
		if(!PQntuples(result)) break;
		if	(flag)
			{	if	(!S_ISREG(strtol(PQgetvalue(result,0,1),NULL,10)))
					{	PQclear(result);
						continue; }
				fputs(PQgetvalue(result,0,2),stdout); }
		printf("%s%c",PQgetvalue(result,0,0),'\0');
		PQclear(result); }
	PQclear(result);
	result=PQexec(db,"close paths_cursor");
	SQLCHECK(db,result,PGRES_COMMAND_OK,err0);
	PQclear(result);
	result=PQexec(db,"end");
	SQLCHECK(db,result,PGRES_COMMAND_OK,err0);
	PQclear(result);
	PQfinish(db);
	exit(EXIT_SUCCESS);
	err0:	PQclear(result);
		PQfinish(db);
		exit(EXIT_FAILURE); }

/*IN GOD WE TRVST.*/
