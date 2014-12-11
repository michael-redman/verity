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
	SHA256_Init(&hash_context);
	while	((bytes_read=fread(buf,1,BUF_SIZE,stream)))
		SHA256_Update(&hash_context,buf,bytes_read);
	SHA256_Final(hash,&hash_context);
	if (ferror(stream) || fclose(stream)) { perror(path); AT; return -1; }
	return 0; }

/* int main (int argc, char ** argv){
	unsigned char hash[SHA256_DIGEST_LENGTH];
	if (argc<2) { fputs("Specify a file!\n",stderr); return -1; }
	unsigned int i;
	if (sha256_of_file(argv[1],hash)) { AT; return -1; }
	for (i=0;i<SHA256_DIGEST_LENGTH;i++) printf("%02x",hash[i]);
	fputs("\n",stdout);
	return 0; } */

//IN GOD WE TRVST.
