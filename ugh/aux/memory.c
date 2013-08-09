#include "memory.h"

aux_pool_t *aux_pool_init(size_t size)
{
	aux_pool_t *pool;

	if (size <= sizeof(*pool))
	{
		size = AUX_POOL_PAGESIZE;
	}

	pool = malloc(size);                        /* [posix_]memalign(AUX_POOL_MEMALIGN, size) */
	if (NULL == pool) return NULL;

	pool->data = (char *) pool + sizeof(*pool);
	pool->last = (char *) pool + size;

	pool->curr = pool;
	pool->next = NULL;

	pool->link = 1;

	return pool;
}

void aux_pool_link(aux_pool_t *pool)
{
	++pool->link;
}

void aux_pool_drop(aux_pool_t *pool)
{
	if (--pool->link) return;

	pool->curr = pool;

	for (; pool; pool = pool->next)
	{
		pool->data = (char *) pool + sizeof(*pool);
	}
}

void aux_pool_free(aux_pool_t *pool)
{
	if (--pool->link) return;

	aux_pool_t *next;

	for (next = pool->next; next; pool = next,
		 next = pool->next)
	{
		free(pool);
	}

	free(pool);
}

static
void *aux_pool_huge(aux_pool_t *pool, size_t size)
{
	void *dest;
	aux_pool_t *next;

	next = aux_pool_init(size + sizeof(*pool));
	if (NULL == next) return NULL;

	dest = next->data;
	next->data += size;

	/* pool->curr->next = next; */
	/* pool->curr = next; */

	next->next = pool->next; /* huge pages are allocated just after the first page */
	pool->next = next;

	return dest;
}

static
void *aux_pool_page(aux_pool_t *pool, size_t size)
{
	void *dest;
	aux_pool_t *next;

	next = aux_pool_init(aux_pool_size(pool));
	if (NULL == next) return NULL;

	dest = next->data;
	next->data += size;

	/* 
	 * TODO
	 * 
	 * Здесь мы теряем хвост страницы, не бегая по списку страниц с проверкой
	 * размера оставшегося хвоста (или количества раз, когда мы эту страницу
	 * уже пробегали, как щас сделано в nginx).
	 *
	 */

	/*
	 * Check if pool->curr->next is not NULL (which can occur if it was
	 * allocated as huge page)
	 */

	for (; pool->curr->next; pool->curr = pool->curr->next)
	{
		/* void */
	}

	pool->curr->next = next;
	pool->curr = next;

	return dest;
}

void *aux_pool_nalloc(aux_pool_t *pool, size_t size)
{
	char *dest;
	aux_pool_t *curr;

	if (size > aux_pool_size(pool) - sizeof(*pool))
	{
		return aux_pool_huge(pool, size);
	}

	for (curr = pool->curr; curr; curr = curr->next)
	{
		dest = curr->data;

		if (dest + size <= curr->last)
		{
			curr->data = dest + size;
			return dest;
		}
	}

	return aux_pool_page(pool, size);
}

void *aux_pool_malloc(aux_pool_t *pool, size_t size)
{
	char *dest;
	aux_pool_t *curr;

	if (size > aux_pool_size(pool) - sizeof(*pool))
	{
		return aux_pool_huge(pool, size);
	}

	for (curr = pool->curr; curr; curr = curr->next)
	{
		dest = aux_align_ptr(curr->data, AUX_MEMALIGN);

		if (dest + size <= curr->last)
		{
			curr->data = dest + size;
			return dest;
		}
	}

	return aux_pool_page(pool, size);
}

void *aux_pool_calloc(aux_pool_t *pool, size_t size)
{
	void *dest;

	dest = aux_pool_malloc(pool, size);

	if (NULL != dest)
	{
		aux_clrmem(dest, size);
	}

	return dest;
}

char *aux_pool_strdup(aux_pool_t *pool, strp data)
{
	char *dest;

	dest = aux_pool_nalloc(pool, data->size);

	if (NULL != dest)
	{
		memcpy(dest, data->data, data->size);
	}

	return dest;
}

char *aux_pool_memdup(aux_pool_t *pool, const char *data, size_t size)
{
	char *dest;

	dest = aux_pool_nalloc(pool, size);

	if (NULL != dest)
	{
		memcpy(dest, data, size);
	}

	return dest;
}

