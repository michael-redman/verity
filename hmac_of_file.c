#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <stdio.h>

#define BUF_SIZE 1024

int hmac_of_file
	(
		const unsigned char * const key,
		const unsigned int key_len,
		const char * const path,
		unsigned char digest[SHA_DIGEST_LENGTH])
	{
		HMAC_CTX context;
		unsigned char buf[BUF_SIZE];
		FILE *stream;
		int bytes_read;
		HMAC_CTX_init(&context);
		if(!(stream=fopen(path,"rb"))) {perror(path); goto err0;}
		HMAC_Init_ex(&context,key,key_len,EVP_sha1(),NULL); /* HMAC_Init_ex is declared void in the local headers but in the Internet docs returns int for error reporting */
		while(!feof(stream)){
			bytes_read=fread(buf,1,BUF_SIZE,stream);
			if(ferror(stream)){ perror(path); goto err1; }
			if	(bytes_read>0)
				HMAC_Update(&context,buf,bytes_read); /* HMAC_Update also void on local system but returns int according to Internet docs */ }
		if(fclose(stream)){ perror(path); goto err0; }
		HMAC_Final(&context,digest,NULL); /* HMAC_Final same */
		HMAC_CTX_cleanup(&context);
		return 0;
		err1:	if (fclose(stream)) perror(path);
		err0:	HMAC_CTX_cleanup(&context);
			return -1; }

/*IN GOD WE TRVST.*/
