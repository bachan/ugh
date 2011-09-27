#include "ugh.h"

ugh_upstream_t *ugh_upstream_add(ugh_config_t *cfg, const char *name, size_t size)
{
	void **dest;
	ugh_upstream_t *u;

	dest = JudyLIns(&cfg->upstreams_hash, aux_hash_key(name, size), PJE0);
	if (PJERR == dest) return NULL;

	u = aux_pool_calloc(cfg->pool, sizeof(*u));
	if (NULL == u) return NULL;

	*dest = u;

	return u;
}

ugh_upstream_t *ugh_upstream_get(ugh_config_t *cfg, const char *name, size_t size)
{
	void **dest;

	dest = JudyLGet(cfg->upstreams_hash, aux_hash_key(name, size), PJE0);
	if (PJERR == dest || NULL == dest) return NULL;

	return *dest;
}

