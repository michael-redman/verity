#define USE "list_paths [-p /path/prefix] [-t as-of-time_t] 'db connection string'"

#define _GNU_SOURCE

#include <libpq-fe.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "err.h"

#define PREFIX_SQL " and substr(paths.path,1,%u)='%s' "

#define SQL_DECLARE	"declare paths_cursor cursor for select paths.path "

#ifndef CORRELATED_SUBQUERY
#define JOIN1			"from "\
				"paths "\
				"join "\
					"(select "\
						"path, max(xtime) as max_xtime "\
						"from paths "\
						"where "\
							"xtime<=%ld "
							//possible PREFIX_SQL
#define JOIN2					"group by path) "\
						"as max_xtimes "\
						"on max_xtimes.path=paths.path and max_xtimes.max_xtime=paths.xtime "
#define JOIN3				"join inodes on inodes.device=paths.device and inodes.inode=paths.inode and inodes.ctime=paths.ctime "
#define JOIN4		"where "\
				"paths.device is not null"
				//possible PREFIX_SQL
#else
#define SQ0	"from paths where device is not null and not exists (select * from paths as alias where alias.path=paths.path and alias.xtime>paths.xtime and alias.xtime<=%ld) "
		//possible PREFIX_SQL
#endif

int main(int argc, char ** argv){
	PGconn *db;
	PGresult *result;
	unsigned int l;
	char *prefix=NULL, *prefix_escaped, *sql, *sql_f;
	time_t t=time(NULL);
	int c;
	while ((c=getopt(argc,argv,"ip:t:"))!=-1){ switch(c){
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
			if	(!(prefix_escaped=malloc(2*l+1)))
				{ fputs("malloc failed\n",stderr); goto err0; }
			PQescapeStringConn(db,prefix_escaped,prefix,l,NULL);
			#ifndef CORRELATED_SUBQUERY
				sql_f=SQL_DECLARE JOIN1 PREFIX_SQL JOIN2 JOIN4 PREFIX_SQL;
			#else
				sql_f=SQL_DECLARE SQ0 PREFIX_SQL;
			#endif
			}
		else{
			#ifndef CORRELATED_SUBQUERY
				sql_f=SQL_DECLARE JOIN1 JOIN2 JOIN4;
			#else
				sql_f=SQL_DECLARE SQ0;
			#endif
			}
		if	(prefix)
			{	 if	(
						#ifndef CORRELATED_SUBQUERY
							asprintf(&sql,sql_f,t,l,prefix_escaped,l,prefix_escaped)
						#else
							asprintf(&sql,sql_f,t,l,prefix_escaped)
						#endif
						==-1)
					{	fputs("asprintf failed",stderr);
						free(prefix_escaped);
						goto err0; }
				free(prefix_escaped); }
			else{
				if	(
						#ifndef CORRELATED_SUBQUERY
							asprintf(&sql,sql_f,t)
						#else
							asprintf(&sql,sql_f,t)
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
		result=PQexec(db,"fetch paths_cursor");
		SQLCHECK(db,result,PGRES_TUPLES_OK,err0);
		if(!PQntuples(result)) break;
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
