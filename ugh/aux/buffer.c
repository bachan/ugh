#include <stdarg.h>
#include <stdio.h>
#include "buffer.h"

int aux_buffer_init(aux_buffer_t *b, aux_pool_t *pool, size_t size)
{
	b->data = aux_pool_nalloc(pool, size);
	if (NULL == b->data) return -1;

	b->size = size;
	b->rpos = 0;
	b->wpos = 0;

	return 0;
}

int aux_buffer_memcpy(aux_buffer_t *b, aux_pool_t *pool, char *data, size_t size)
{
	if (b->size < b->wpos + size)
	{
		char *old_data = b->data;

		b->size = b->size < size ? b->size + size : b->size * 2;
		b->data = aux_pool_nalloc(pool, b->size);
		if (NULL == b->data) return -1;

		memcpy(b->data, old_data, b->wpos);
	}

	memcpy(b->data + b->wpos, data, size);
	b->wpos += size;

	return 0;
}

int aux_buffer_printf(aux_buffer_t *b, aux_pool_t *pool, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	size_t size = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);

	if (b->size < b->wpos + size + 1) /* we need +1 cause vsnprintf writes terminating null */
	{
		char *old_data = b->data;

		b->size = b->size < size + 1 ? b->size + size + 1 : b->size * 2;
		b->data = aux_pool_nalloc(pool, b->size);
		if (NULL == b->data) return -1;

		memcpy(b->data, old_data, b->wpos);
	}

	va_start(ap, fmt);
	b->wpos += vsnprintf(b->data + b->wpos, b->size - b->wpos, fmt, ap);
	va_end(ap);

	return 0;
}

