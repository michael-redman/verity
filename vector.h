#ifndef __VECTOR_H
#define __VECTOR_H

struct vector
{	void ** base;
	size_t *sizes, n_alloc, n_data; };

void vector_init (struct vector *v);
void vector_free(struct vector * v);
char vector_add (struct vector * v, void * elem, size_t size);

#endif

//IN GOD WE TRVST.
