#include "ugh.h"

strp ugh_variable_var_(ugh_client_t *c, const char *name, size_t size, void *data)
{
	return ugh_client_getvar(c, name + 4, size - 4);
}

strp ugh_variable_arg_(ugh_client_t *c, const char *name, size_t size, void *data)
{
	return ugh_client_getarg(c, name + 4, size - 4);
}

strp ugh_variable_http_(ugh_client_t *c, const char *name, size_t size, void *data)
{
	ugh_header_t *h = ugh_client_header_get(c, name + 5, size - 5);
	if (NULL == h) return &aux_empty_string;

	return &h->value;
}

strp ugh_variable_cookie_(ugh_client_t *c, const char *name, size_t size, void *data)
{
	return ugh_client_cookie_get(c, name + 7, size - 7);
}

strp ugh_variable_body_(ugh_client_t *c, const char *name, size_t size, void *data)
{
	return ugh_client_body_getarg(c, name + 5, size - 5);
}

strp ugh_variable_hash_(ugh_client_t *c, const char *name, size_t size, void *data)
{
	strp val = ugh_get_varvalue(c, name + 5, size - 5);
	uintptr_t key = aux_hash_key(val->data, val->size);

	strp res = aux_pool_malloc(c->pool, sizeof(*res));
	if (NULL == res) return &aux_empty_string;

	res->data = aux_pool_nalloc(c->pool, 32);
	if (NULL == res->data) return &aux_empty_string;

	res->size = snprintf(res->data, 32, "%"PRIuPTR, key);

	return res;
}

strp ugh_variable_c0_(ugh_client_t *c, const char *name, size_t size, void *data)
{
	strp val = ugh_get_varvalue(c, name + 3, size - 3);
	if (1 > val->size) return &aux_empty_string;

	strp res = aux_pool_malloc(c->pool, sizeof(*res));
	if (NULL == res) return &aux_empty_string;

	res->data = val->data;
	res->size = 1;

	return res;
}

strp ugh_variable_cl_(ugh_client_t *c, const char *name, size_t size, void *data)
{
	strp val = ugh_get_varvalue(c, name + 3, size - 3);
	if (1 > val->size) return &aux_empty_string;

	strp res = aux_pool_malloc(c->pool, sizeof(*res));
	if (NULL == res) return &aux_empty_string;

	res->data = val->data + val->size - 1;
	res->size = 1;

	return res;
}

ugh_variable_t ugh_variables [] =
{
	{ aux_string("c0_")    , ugh_variable_c0_    , NULL },
	{ aux_string("cl_")    , ugh_variable_cl_    , NULL },
	{ aux_string("hash_")  , ugh_variable_hash_  , NULL },
	{ aux_string("arg_")   , ugh_variable_arg_   , NULL },
#if 1
	{ aux_string("var_")   , ugh_variable_var_   , NULL },
#endif
	{ aux_string("http_")  , ugh_variable_http_  , NULL },
	{ aux_string("cookie_"), ugh_variable_cookie_, NULL },
	{ aux_string("body_")  , ugh_variable_body_  , NULL },
	{ aux_null_string      , NULL                , NULL },
};

ugh_variable_t *ugh_set_variable(ugh_config_t *cfg, const char *name, size_t size, ugh_variable_t *var)
{
	void **dest = JudyLIns(&cfg->vars_hash, aux_hash_key(name, size), PJE0);
	if (PJERR == dest) return NULL;

	*dest = var;

	return var;
}

ugh_variable_t *ugh_get_variable(ugh_config_t *cfg, const char *name, size_t size)
{
	void **dest = JudyLGet(cfg->vars_hash, aux_hash_key(name, size), PJE0);
	if (PJERR == dest || NULL == dest) return NULL;

	return *dest;
}

void ugh_idx_variable(ugh_config_t *cfg, const char *name, size_t size)
{
	ugh_variable_t *var;

	for (var = ugh_variables; var->name.data; ++var)
	{
		if (0 != strncmp(name, var->name.data, var->name.size))
			continue;

		/* TODO return error if some component was not found */

		ugh_idx_variable(cfg, name + var->name.size, size - var->name.size);
		ugh_set_variable(cfg, name, size, var);

		break;
	}
}

strp ugh_get_varvalue(ugh_client_t *c, const char *name, size_t size)
{
	ugh_variable_t *var = ugh_get_variable(c->s->cfg, name, size);
	if (NULL == var) return &aux_empty_string;

	return var->handle(c, name, size, var->data);
}

