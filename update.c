#define USE "verity_update [-a] 'db conn string' < path_list"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <arpa/inet.h>
#include <endian.h>
#include <libpq-fe.h>
#include <linux/limits.h>
#include <openssl/engine.h>
#include <openssl/sha.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "err.h"

extern char sha256_of_file (char const * const, unsigned char[SHA256_DIGEST_LENGTH]);

void hexbytes_print
        (       const unsigned char * const binary
                , const unsigned int length
                , char * const text)
        {       unsigned int i;
                for     (i=0;i<length;i++)
                        sprintf(&text[2*i],"%02hhx",binary[i]); }

static int insert_path
	(PGconn *db, char const * const path, uint64_t device, uint64_t inode, uint64_t ctime)
	//Returns 0 on success.  device,inode,ctime should already be big-endian
	{
		PGresult *result=PQexecParams(db,
			"insert into paths(path,xtime,device,inode,ctime) values ($1,extract(epoch from now()),$2,$3,$4)",
			4,NULL, (char const * const []){
				path,
				(char const * const)&device,
				(char const * const)&inode,
				(char const * const)&ctime},
			(const int []){
				0,
				sizeof(uint64_t),
				sizeof(uint64_t),
				sizeof(uint64_t)},
				(const int []){0,1,1,1},1);
		if	(PQresultStatus(result)!=PGRES_COMMAND_OK) 
			{	fputs(PQerrorMessage(db),stderr); AT;
				PQclear(result); return -1; }
		PQclear(result);
		return 0; }

int main(int argc, char ** argv){
	PGconn *db;
	PGresult *result;
	char *path=NULL, inode_content[PATH_MAX+1];
	unsigned char hmac_binary[SHA256_DIGEST_LENGTH], flags=0;
	uint64_t device, inode, ctime, mtime;
	uint32_t mode, user, group;
	struct stat st;
	size_t s,path_len=0;
	if(getopt(argc,argv,"a")=='a') flags=1;
	if (argc-optind!=1) { fputs(USE "\n",stderr); exit(EXIT_FAILURE); }
	db=PQconnectdb(argv[optind]);
	if(PQstatus(db)!=CONNECTION_OK){ fputs(PQerrorMessage(db),stderr); AT; return 1; }
	while(!feof(stdin)){
		if (getdelim(&path,&path_len,'\0',stdin)==-1) break;
		result=PQexec(db,"begin");
		SQLCHECK(db,result,PGRES_COMMAND_OK,err1);
		PQclear(result);
		if	(lstat(path,&st))
			{	//done if path is DB is also dead
				result=PQexecParams(db,
					"select device from paths where path=$1 and xtime=(select max(xtime) from paths where path=$1)",
					1,NULL,(char const * const []){path},
					(const int []){0}, (const int []){0},1);
				SQLCHECK(db,result,PGRES_TUPLES_OK,err1);
				if (!PQntuples(result) || PQgetisnull(result,0,0)) goto end1;
				PQclear(result);
				result=PQexecParams(db,
					"insert into paths(path,xtime,device,inode,ctime) values ($1,extract(epoch from now()),null,null,null)",
					1,NULL,(char const * const []){path},
					(const int []){0}, (const int []){0},1);
				SQLCHECK(db,result,PGRES_COMMAND_OK,err1);
				PQclear(result); }
			else{	//fs has inode at path
				device=htobe64(st.st_dev);
				inode=htobe64(st.st_ino);
				ctime=htobe64(st.st_ctime);
				result=PQexecParams(db,
					"select mode,uid,gid,mtime,content from inodes where device=$1 and inode=$2 and ctime=$3",
					3,NULL,
					(char const * const []){
						(char const * const)&device,
						(char const * const)&inode,
						(char const * const)&ctime},
					(const int []){
						sizeof(uint64_t),
						sizeof(uint64_t),
						sizeof(uint64_t) },
		 			(const int []){1,1,1},1);
				SQLCHECK(db,result,PGRES_TUPLES_OK,err1);
				if	(PQntuples(result))
					{	//inode exists in DB
						if	(flags)
							{	mode=ntohl(*(uint32_t*)PQgetvalue(result,0,0));
								user=ntohl(*(uint32_t*)PQgetvalue(result,0,1));
								group=ntohl(*(uint32_t*)PQgetvalue(result,0,2));
								mtime=be64toh(*(uint64_t*)PQgetvalue(result,0,3));
								if	(	st.st_mode != mode
										|| st.st_uid != user
										|| st.st_gid != group
										|| st.st_mtime != mtime)
									{
										fprintf(stderr,
											"Error, data mismatch, not updating database, at device %lu inode %lu ctime %ld: db has mode=%u user=%u group=%u mtime=%lu, fs has mode=%u user=%u group=%u mtime=%lu (path %s)",
											(unsigned long)device, (unsigned long)inode, (unsigned long)ctime, (unsigned int)mode, (unsigned int)user, (unsigned int)group, (unsigned long)mtime, (unsigned int)st.st_mode, (unsigned int)st.st_uid, (unsigned int)st.st_gid,(unsigned long)st.st_mtime,path);
										goto err1; }
								if	(S_ISLNK(mode))
									{	if	(PQgetisnull(result,0,4))
											{	fprintf(stderr,"Error: inode record content should not be null for inode type symlink, at device %lu inode %lu ctime %lu path %s\n",(unsigned long)st.st_dev,st.st_ino,st.st_ctime,path);
												goto err1; }
										s=readlink(path,inode_content,PATH_MAX);
										if (s==-1) {perror(path); AT; goto err1; }
										inode_content[s]='\0';
										if	(strcmp(inode_content,PQgetvalue(result,0,4)))
											{	fprintf(stderr,"Error: symlink target mismatch, at device %lu inode %lu ctime %lu path %s, fs has %s, db has %s\n",(unsigned long)st.st_dev,st.st_ino,st.st_ctime,path,inode_content,PQgetvalue(result,0,4));
												goto err1; }}
								if	(S_ISREG(mode))
									{	if	(PQgetisnull(result,0,4))
											{	fprintf(stderr,"Error: inode record content should not be null for inode type regular file, at device %lu inode %lu ctime %lu path %s\n",(unsigned long)st.st_dev,st.st_ino,st.st_ctime,path);
												goto err1; }
										if (sha256_of_file(path,hmac_binary)) goto err1;
										hexbytes_print(hmac_binary,SHA256_DIGEST_LENGTH,inode_content);
										if	(strcmp(inode_content,PQgetvalue(result,0,4)))
											{	fprintf(stderr,"Error: regular file hmac mismatch, at device %lu inode %lu ctime %lu path %s, fs has %s, db has %s\n",(unsigned long)st.st_dev,st.st_ino,st.st_ctime,path,inode_content,PQgetvalue(result,0,4));
												goto err1; }}}
						PQclear(result);
						//done if paths table in db has entry for correct inode
						result=PQexecParams(db,
							"select device,inode,ctime from paths where path=$1 and xtime=(select max(xtime) from paths where path=$1)",
							1,NULL,(char const * const []){path},
							(const int []){0}, (const int []){0},1);
						SQLCHECK(db,result,PGRES_TUPLES_OK,err1);
						device=htobe64(st.st_dev);
						inode=htobe64(st.st_ino);
						ctime=htobe64(st.st_ctime);
						if	(	PQntuples(result)
								&& !PQgetisnull(result,0,0)
								&& device==*(uint64_t*)PQgetvalue(result,0,0)
								&& inode==*(uint64_t*)PQgetvalue(result,0,1)
								&& ctime==*(uint64_t*)PQgetvalue(result,0,2))
							goto end1;
						PQclear(result);
						if (insert_path(db,path,device,inode,ctime)) goto err0; }
					else{	//Inode not in DB
						PQclear(result);
						if (S_ISCHR(st.st_mode)||S_ISBLK(st.st_mode)||S_ISFIFO(st.st_mode)||S_ISSOCK(st.st_mode)) goto end0;
						device=htobe64(st.st_dev);
						inode=htobe64(st.st_ino);
						ctime=htobe64(st.st_ctime);
						mode=htonl(st.st_mode);
						user=htonl(st.st_uid);
						group=htonl(st.st_gid);
						mtime=htobe64(st.st_mtime);
						if	(S_ISDIR(st.st_mode))
							result=PQexecParams(db,
								"insert into inodes (device,inode,ctime,mode,uid,gid,mtime,content) values ($1,$2,$3,$4,$5,$6,$7,null)",
								7,NULL,
								(char const * const []){
									(char const * const)&device,
									(char const * const)&inode,
									(char const * const)&ctime,
									(char const * const)&mode,
									(char const * const)&user,
									(char const * const)&group,
									(char const * const)&mtime},
								(const int []){
									sizeof(uint64_t),
									sizeof(uint64_t),
									sizeof(uint64_t),
									sizeof(uint32_t),
									sizeof(uint32_t),
									sizeof(uint32_t),
									sizeof(uint64_t)},
								(const int []){1,1,1,1,1,1,1},1);
							else{	if	(S_ISLNK(st.st_mode))
									{	s=readlink(path,inode_content,PATH_MAX);
										if (s==-1) {perror(path); AT; goto err0; }
										inode_content[s]='\0'; }
								if	(S_ISREG(st.st_mode))
									{	if (sha256_of_file(path,hmac_binary)) goto err0;
										hexbytes_print(hmac_binary,SHA256_DIGEST_LENGTH,inode_content); }
								result=PQexecParams(db,
									"insert into inodes (device,inode,ctime,mode,uid,gid,mtime,content) values ($1,$2,$3,$4,$5,$6,$7,$8)",
									8,NULL,
									(char const * const []){
										(char const * const)&device,
										(char const * const)&inode,
										(char const * const)&ctime,
										(char const * const)&mode,
										(char const * const)&user,
										(char const * const)&group,
										(char const * const)&mtime,
										inode_content},
									(const int []){
										sizeof(uint64_t),
										sizeof(uint64_t),
										sizeof(uint64_t),
										sizeof(uint32_t),
										sizeof(uint32_t),
										sizeof(uint32_t),
										sizeof(uint64_t),
										0},
									(const int []){1,1,1,1,1,1,1,0},1); }
						SQLCHECK(db,result,PGRES_COMMAND_OK,err1);
						PQclear(result);
						if (insert_path(db,path,device,inode,ctime)) goto err0; }}
		result=PQexec(db,"end");
		SQLCHECK(db,result,PGRES_COMMAND_OK,err1);
		PQclear(result);
		continue;
		end1:	PQclear(result);
		end0:	result=PQexec(db,"end");
			SQLCHECK(db,result,PGRES_COMMAND_OK,err1);
			PQclear(result); }
	if (ferror(stdin)) { perror("stdin"); AT; goto err0; }
	free(path);
	PQfinish(db);
	ENGINE_cleanup();
	exit(EXIT_SUCCESS);
	err1:	PQclear(result);
	err0:	free(path);
		PQfinish(db);
		ENGINE_cleanup();
		exit(EXIT_FAILURE); }

/*IN GOD WE TRVST.*/
