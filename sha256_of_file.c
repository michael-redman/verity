#include <openssl/sha.h>
#include <stdio.h>

#define AT fprintf(stderr,"at " __FILE__ ": %u\n",__LINE__)

#define BUF_SIZE 0xffff

char sha256_of_file
(char const * const path, unsigned char hash[SHA256_DIGEST_LENGTH])
{	unsigned char buf[BUF_SIZE];
	int bytes_read;
	SHA256_CTX hash_context;
	FILE *stream=fopen(path,"rb");
	if (!stream) { perror(path); AT; return -1; }
	if (SHA256_Init(&hash_context)!=1) { AT; goto label0; }
	while	(!feof(stream))
		{	bytes_read=fread(buf,1,BUF_SIZE,stream);
			if (ferror(stream)) { perror(path); AT; goto label0; }
			if (!bytes_read) break;
			if	(SHA256_Update(&hash_context,buf,bytes_read)!=1)
				{ AT; goto label0; } }
	if (fclose(stream)) { perror(path); AT; return 1; }
	SHA256_Final(hash,&hash_context);
	return 0;
	label0:	if (fclose(stream)) perror(path);
		return 1; }

/* int main (int argc, char ** argv){
	unsigned char hash[SHA256_DIGEST_LENGTH];
	if (argc<2) { fputs("Specify a file!\n",stderr); return -1; }
	unsigned int i;
	if (sha256_of_file(argv[1],hash)) { AT; return -1; }
	for (i=0;i<SHA256_DIGEST_LENGTH;i++) printf("%02x",hash[i]);
	fputs("\n",stdout);
	return 0; } */

//IN GOD WE TRVST.
