#include <stdlib.h>
#include <string.h>

#include "vector.h"

void vector_init (struct vector *v)
{	v->base=NULL;
	v->sizes=NULL;
	v->n_alloc=0;
	v->n_data=0; }

void vector_free(struct vector * v)
{	size_t i;
	for (i=0;i<v->n_data;i++) free(v->base[i]);
	free(v->base);
	free(v->sizes); }

char vector_add (struct vector * v, void * elem, size_t size)
{	void ** p;
	size_t * q;
	if	(v->n_alloc==v->n_data)
		{	v->n_alloc=1+3*v->n_alloc;
			p=realloc(v->base,v->n_alloc*sizeof(void *));
			if (!p) return 1;
			v->base=p;
			q=realloc(v->sizes,v->n_alloc*sizeof(size_t));
			if (!q) return 1;
			v->sizes=q; }
	v->base[v->n_data]=malloc(size);
	if (!v->base[v->n_data]) return 1;
	v->n_data++;
	memcpy(v->base[v->n_data-1],elem,size);
	v->sizes[v->n_data-1]=size;
	return 0; }

//IN GOD WE TRVST.
