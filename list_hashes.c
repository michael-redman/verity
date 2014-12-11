#define USE "list_hashes [-p /path/prefix] [-t as-of-time_t] 'db connection string'"

#define _GNU_SOURCE

#include <libpq-fe.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "err.h"

int main(int argc, char ** argv){
	PGconn *db;
	PGresult *result;
	unsigned int l;
	char *prefix=NULL, *prefix_escaped, *sql;
	time_t t=time(NULL);
	int c;
	while ((c=getopt(argc,argv,"p:t:"))!=-1){ switch(c){
		case 'p': prefix=optarg; break;
		case 't': t=strtoll(optarg,NULL,10); break;
		default: fputs(USE,stderr); exit(EXIT_FAILURE); }}
	if (!(argc-optind)) { fputs(USE,stderr); exit(EXIT_FAILURE); }
	db=PQconnectdb(argv[optind]);
	if(PQstatus(db)!=CONNECTION_OK){
		fputs(PQerrorMessage(db),stderr);
		exit(EXIT_FAILURE); }
	result=PQexec(db,"begin");
	SQLCHECK(db,result,PGRES_COMMAND_OK,err0);
	if	(prefix)
		{	l=strlen(prefix);
			//PQExecParams does not work with cursor under PG 7.4 - throws error - so use escape strings instead - see http://www.postgresql.org/message-id/20050117165837.GA80669@winnie.fuhr.org
			if
				(!(prefix_escaped=malloc(2*l+1)))
				{ fputs("malloc failed\n",stderr); goto err0; }
			PQescapeStringConn(db,prefix_escaped,prefix,l,NULL);
			if	(
					#ifndef CORRELATED_SUBQUERY
					asprintf(&sql,"declare hmacs cursor for select content from paths join (select path, max(xtime) as max_xtime from paths where substr(path,1,%u)='%s' and xtime<=%ld group by path) as max_xtimes on max_xtimes.path=paths.path and max_xtimes.max_xtime=paths.xtime join inodes on inodes.device=paths.device and inodes.inode=paths.inode and inodes.ctime=paths.ctime where mode-mode%%4096=32768 and substr(paths.path,1,%u)='%s'",l,prefix_escaped,t,l,prefix_escaped)
					#else
					asprintf(&sql,
//"declare hmacs cursor for select content from inodes where mode-mode%%4096=32768 and exists (select * from paths where paths.device=inodes.device and paths.inode=inodes.inode and paths.ctime=inodes.ctime and substr(path,1,%u)='%s' and xtime=(select max(xtime) from paths as alias where alias.path=paths.path and xtime<=%ld))",l,prefix_escaped,t) //if we skip the nulls in the output we can use a query that's half as slow
"declare hmacs cursor for select "
	"(select content from inodes where mode-mode%%4096=32768 and inodes.device=recent_paths.device and inodes.inode=recent_paths.inode and inodes.ctime=recent_paths.ctime) "
	"from (select device,inode,ctime from paths where device is not null and xtime=(select max(xtime) from paths as alias where alias.path=paths.path and alias.xtime<=%ld) and substr(path,1,%u)='%s') as recent_paths"
					,t,l,prefix_escaped)
					#endif
					==-1)
				{	fputs("asprintf failed",stderr);
					free(prefix_escaped);
					goto err0; }
			free(prefix_escaped); }
		else{
			if	(
					#ifndef CORRELATED_SUBQUERY
					asprintf(&sql,"declare hmacs cursor for select content from paths join (select path, max(xtime) as max_xtime from paths where xtime<=%ld group by path) as max_xtimes on max_xtimes.path=paths.path and max_xtimes.max_xtime=paths.xtime join inodes on inodes.device=paths.device and inodes.inode=paths.inode and inodes.ctime=paths.ctime where mode-mode%%4096=32768",t)
					#else
					asprintf(&sql,
"declare hmacs cursor for select "
	"(select content from inodes where mode-mode%%4096=32768 and inodes.device=recent_paths.device and inodes.inode=recent_paths.inode and inodes.ctime=recent_paths.ctime) "
	"from (select device,inode,ctime from paths where device is not null and xtime=(select max(xtime) from paths as alias where alias.path=paths.path and alias.xtime<=%ld)) as recent_paths"
					,t)
					#endif
					==-1)
				{	fputs("asprintf failed",stderr);
					goto err0; }}
	PQclear(result);
	result=PQexec(db,sql);
	free(sql);
	SQLCHECK(db,result,PGRES_COMMAND_OK,err0);
	PQclear(result);
	while(1){
		result=PQexec(db,"fetch hmacs");
		SQLCHECK(db,result,PGRES_TUPLES_OK,err0);
		if(!PQntuples(result)) break;
		#ifdef CORRELATED_SUBQUERY
			if	(PQgetisnull(result,0,0))
				{ PQclear(result); continue; }
			#endif
		puts(PQgetvalue(result,0,0));
		PQclear(result); }
	PQclear(result);
	result=PQexec(db,"close hmacs");
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
