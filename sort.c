#define USE "verity_diff ... | verity_sort\n"

#include <limits.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fgetsnull.h>

#include "at.h"
#include "vector.h"

int compar (const void *a, const void *b) { return strcmp(
	&(*(char **)a)[65],
	&(*(char **)b)[65]); }

#define REC_LEN PATH_MAX+2*SHA256_DIGEST_LENGTH+2

int main
(	int argc, char ** argv)
{	struct vector data;
	char buf[REC_LEN];
	size_t i;
	vector_init(&data);
	while	(!feof(stdin))
		{	if (!fgetsnull(buf,REC_LEN,stdin)) break;
			if	(vector_add(&data,buf,strlen(buf)+1))
				{	fputs("vector_add failed",stdout);
					AT_ERR;
					goto l0; }}
	if (ferror(stdin)) { perror("stdin"); AT_ERR; goto l0; }
	qsort(data.base,data.n_data,sizeof(void *),compar);
	for	(i=0;i<data.n_data;i++)
		if	(	fputs((char *)(data.base[i]),stdout)==EOF
				|| fputc('\0',stdout)==EOF)
			{ perror("stdout"); AT_ERR; goto l0; }
	vector_free(&data);
	return 0;
	l0:	vector_free(&data);
		return 1; }

//IN GOD WE TRVST.
