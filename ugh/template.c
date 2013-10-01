#include <ctype.h>
#include "ugh.h"

#define S_STR 0
#define S_VAR 1

int ugh_template_compile(ugh_template_t *t, char *data, size_t size, ugh_config_t *cfg)
{
	char *b = data;
	char *p = data;
	char *e = data + size;

	unsigned char state = S_STR;

	strt chunks [UGH_TEMPLATE_MAX_CHUNKS];
	size_t chunks_size = 0;

	for (; p < e; ++p)
	{
		char ch = *p;

		if (S_STR == state)
		{
			if ('$' == ch)
			{
				if (b < p && chunks_size < UGH_TEMPLATE_MAX_CHUNKS)
				{
					chunks[chunks_size].data = b;
					chunks[chunks_size].size = p - b;
					chunks_size++;
				}

				state = S_VAR;

				b = p;
			}
		}
		else
		{
			if (!isalnum(ch) && '_' != ch)
			{
				if (b < p && chunks_size < UGH_TEMPLATE_MAX_CHUNKS)
				{
					chunks[chunks_size].data = b;
					chunks[chunks_size].size = p - b;
					chunks_size++;
				}

				state = S_STR;

				b = p;
			}
		}
	}

	if (b < p && chunks_size < UGH_TEMPLATE_MAX_CHUNKS)
	{
		chunks[chunks_size].data = b;
		chunks[chunks_size].size = p - b;
		chunks_size++;
	}

	size_t i;

	t->chunks_size = chunks_size;
	t->chunks = aux_pool_malloc(cfg->pool, sizeof(t->chunks[0]) * t->chunks_size);
	if (NULL == t->chunks) return -1;

	for (i = 0; i < t->chunks_size; ++i)
	{
		t->chunks[i].data = chunks[i].data;
		t->chunks[i].size = chunks[i].size;

		if ('$' == chunks[i].data[0])
		{
			ugh_idx_variable(cfg, chunks[i].data + 1, chunks[i].size - 1);
		}
	}

	return 0;
}

strp ugh_template_execute(ugh_template_t *t, ugh_client_t *c)
{
	if (1 == t->chunks_size)
	{
		if ('$' != t->chunks[0].data[0])
		{
			return &t->chunks[0];
		}
		else
		{
			return ugh_get_varvalue(c, t->chunks[0].data + 1, t->chunks[0].size - 1);
		}
	}

	size_t i;

	strp res = aux_pool_malloc(c->pool, sizeof(*res));
	if (NULL == res) return &aux_empty_string;

	res->data = aux_pool_nalloc(c->pool, UGH_TEMPLATE_MAX_LENGTH);
	if (NULL == res->data) return &aux_empty_string;

	res->size = 0;

	for (i = 0; i < t->chunks_size; ++i)
	{
		if ('$' != t->chunks[i].data[0])
		{
			res->size += snprintf(res->data + res->size, UGH_TEMPLATE_MAX_LENGTH - res->size, "%.*s",
				(int) t->chunks[i].size, t->chunks[i].data);
		}
		else
		{
			strp vv = ugh_get_varvalue(c, t->chunks[i].data + 1, t->chunks[i].size - 1);
			res->size += snprintf(res->data + res->size, UGH_TEMPLATE_MAX_LENGTH - res->size, "%.*s",
				(int) vv->size, vv->data);
		}
	}

	return res;
}

