#define USE "verity_list [-c] [-f psql_posix_regex] [-t as-of-time_t] 'db connection string'\n"

#define _GNU_SOURCE

#include <arpa/inet.h>
#include <endian.h>
#include <libpq-fe.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "err.h"

/* flag bits
0	-c: only print output for regular files but print hash before path
1	-t: use an as-of time */

int main(int argc, char ** argv){
	PGconn *db;
	PGresult *result;
	char *sql, *regex=NULL, flags=0;
	uint64_t as_of_time;
	int c;
	while ((c=getopt(argc,argv,"cf:t:"))!=-1){ switch(c){
		case 'c': flags|=1; break;
		case 'f': regex=optarg; break;
		case 't':
			flags|=2;
			as_of_time=htobe64(strtoll(optarg,NULL,10));
			break;
		default: fputs(USE,stderr); exit(EXIT_FAILURE); }}
	if (!(argc-optind)) { fputs(USE,stderr); exit(EXIT_FAILURE); }
	db=PQconnectdb(argv[optind]);
	if(PQstatus(db)!=CONNECTION_OK){
		fputs(PQerrorMessage(db),stderr);
		exit(EXIT_FAILURE); }
	result=PQexec(db,"begin");
	SQLCHECK(db,result,PGRES_COMMAND_OK,err0);
	#ifndef CORRELATED_SUBQUERY
	c=asprintf(&sql,
		"declare paths_cursor cursor for "
		"select "
			"paths.path"
			"%s"
		" from"
			"  paths"
			" join ("
				" select"
					" path,"
					" max(xtime) as max_xtime"
				" from paths"
				"%s%s%s%s"
				" group by path)"
				" as max_xtimes on"
					" max_xtimes.path=paths.path"
					" and max_xtimes.max_xtime=paths.xtime"
			"%s"
		" where paths.device is not null"
		, flags&1 ? ", mode, content " : ""
		, regex || flags&2 ? " where" : ""
		, regex ? " path ~ $1" : ""
		, regex && flags&2 ? " and" : ""
		, flags&2 ? (regex ? " xtime<=$2" : " xtime<=$1") : ""
		, flags&1 ? " join inodes on inodes.device=paths.device and inodes.inode=paths.inode and inodes.ctime=paths.ctime" : "");
	#else
	c=asprintf(&sql,
		"declare paths_cursor cursor for"
		" select"
			" paths.path"
			"%s"
		" from paths"
		" where"
			" device is not null "
			"%s"
			" and not exists (select * from paths as alias where"
				" alias.path=paths.path"
				" and alias.xtime>paths.xtime"
				"%s)"
		, flags&1 ? ", (select mode from inodes where inodes.device=paths.device and inodes.inode=paths.inode and inodes.ctime=paths.ctime) as mode, (select content from inodes where inodes.device=paths.device and inodes.inode=paths.inode and inodes.ctime=paths.ctime) as content" : ""
		, regex ? " and path ~ $1" : ""
		, flags&2
			? (regex
				? " and alias.xtime<=$2"
				: " and alias.xtime<=$1")
			: "");
	#endif
	if	(c==-1)
		{ fputs("asprintf failed",stderr); goto err0; }
	PQclear(result);
	if	(!regex && !(flags&2))
		result=PQexecParams(db,sql,0,NULL,NULL,NULL,NULL,1);
		else if	(regex && flags&2)
			result=PQexecParams(db,sql,2,NULL,
				(char const * const[]){
					regex,
					(char const * const)&as_of_time},
				(const int []){ 0, sizeof(uint64_t) },
				(const int []){ 0, 1 }, 1);
			else result=PQexecParams(db,sql,1,NULL,
				(char const * const[]){ regex
					? regex
					: (char const * const)&as_of_time },
				(const int []){ regex ? 0 : sizeof(uint64_t) },
				(const int []){ regex ? 0 : 1 }, 1);
	free(sql);
	SQLCHECK(db,result,PGRES_COMMAND_OK,err0);
	PQclear(result);
	while(1){
		result=PQexec(db,"fetch paths_cursor");
		SQLCHECK(db,result,PGRES_TUPLES_OK,err0);
		if(!PQntuples(result)) break;
		if	(flags&1)
			{	if	(!S_ISREG(strtoll(PQgetvalue(result,0,1),NULL,10))) //We get text back here even though we asked for binary!!! - might be something to do with cursor?
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
