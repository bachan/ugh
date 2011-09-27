#include <stdio.h>
#include "../ugh/aux/memory.h"

int main()
{
	aux_pool_t *pool;

	pool = aux_pool_init(0);

	aux_pool_malloc(pool, 1024 * 512);
	aux_pool_malloc(pool, 1024);
	/* aux_pool_malloc(pool, 1024); */
	/* aux_pool_malloc(pool, 1024); */
	/* aux_pool_malloc(pool, 1024); */
	/* aux_pool_malloc(pool, 1024); */
	/* aux_pool_malloc(pool, 1024); */
	/* aux_pool_malloc(pool, 1024); */
	/* aux_pool_malloc(pool, 1024); */
	/* aux_pool_malloc(pool, 1024); */
	/* aux_pool_malloc(pool, 1024); */
	/* aux_pool_malloc(pool, 1024); */
	/* aux_pool_malloc(pool, 1024); */
	/* aux_pool_malloc(pool, 1024); */
	/* aux_pool_malloc(pool, 1024); */
	/* aux_pool_malloc(pool, 1024); */
	/* aux_pool_malloc(pool, 1024); */
	/* aux_pool_malloc(pool, 1024); */

	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */
	/* printf("%p\n", aux_pool_malloc(pool, 32333)); */
	/* printf("%p\n", aux_pool_malloc(pool, 1024)); */

	aux_pool_free(pool);

	return 0;
}

