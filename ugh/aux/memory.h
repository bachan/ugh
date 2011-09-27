#ifndef __AUX_MEMORY_H__
#define __AUX_MEMORY_H__

#include <stdint.h>
#include <stdlib.h>
#include "string.h"

#define AUX_PAGESIZE 4096
#define AUX_MEMALIGN sizeof(uintptr_t)

#define aux_align(x,a) (((x) + ((a) - 1)) & ~((a) - 1))
#define aux_align_ptr(p,a) ((char *) (((uintptr_t) (p) + ((uintptr_t) (a) - 1)) & ~((uintptr_t) (a) - 1)))

#define AUX_POOL_MEMALIGN 16
#define AUX_POOL_PAGESIZE 16384

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aux_pool
	aux_pool_t;

struct aux_pool
{
	char *data;
	char *last;
	aux_pool_t *curr;
	aux_pool_t *next;
	unsigned int link;
};

aux_pool_t *aux_pool_init(size_t size);
void aux_pool_link(aux_pool_t *pool);
void aux_pool_drop(aux_pool_t *pool);
void aux_pool_free(aux_pool_t *pool);

#define aux_pool_size(p) ((size_t) ((p)->last - ((char *) (p))))

void *aux_pool_nalloc(aux_pool_t *pool, size_t size);  /* non-aligned */
void *aux_pool_malloc(aux_pool_t *pool, size_t size);  /*     aligned */
void *aux_pool_calloc(aux_pool_t *pool, size_t size);  /*     aligned */
char *aux_pool_strdup(aux_pool_t *pool, strp data);    /* non-aligned */

#ifdef __cplusplus
}
#endif

#endif /* __AUX_MEMORY_H__ */
