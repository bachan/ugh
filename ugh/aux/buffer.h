#ifndef __AUX_BUFFER_H__
#define __AUX_BUFFER_H__

#include "config.h"
#include "memory.h"

typedef struct aux_buffer
	aux_buffer_t;

struct aux_buffer
{
	size_t size;
	char  *data;
	size_t rpos;
	size_t wpos;
};

int aux_buffer_init(aux_buffer_t *b, aux_pool_t *pool, size_t size);
#define aux_buffer_reset(b) { (b)->rpos = 0; (b)->wpos = 0; }

#define aux_buffer_strcpy(b, pool, data) aux_buffer_memcpy(b, pool, data, strlen(data))
int aux_buffer_memcpy(aux_buffer_t *b, aux_pool_t *pool, char *data, size_t size);
int aux_buffer_printf(aux_buffer_t *b, aux_pool_t *pool, const char *fmt, ...) AUX_FORMAT(printf, 3, 4);

#endif /* __AUX_BUFFER_H__ */
