#include "ugh.h"

typedef struct
{
	void *hash;
	strt key;
	strt default_value;
} ugh_module_map_conf_t;

/* TODO XXX JudyLFreeArray */

static
int ugh_command_map_handle_line(ugh_config_t *cfg, int argc, char **argv)
{
/* msg("%s -> %s", argv[0], (2 > argc) ? "" : argv[1]); */

	ugh_module_map_conf_t *conf = ugh_module_configs[ugh_module_handles_size - 1]; /* TODO make this API element */

	if (2 > argc)
	{
		conf->default_value.size = strlen(argv[0]);
		conf->default_value.data = argv[0];

		return 0;
	}

	strp value;

	value = (strp) aux_pool_malloc(cfg->pool, sizeof(*value));
	if (NULL == value) return -1;

	value->size = strlen(argv[1]);
	value->data = argv[1];

	void **dest;

	dest = JudyLIns(&conf->hash, aux_hash_key_nt(argv[0]), PJE0);
	if (PJERR == dest) return -1;

	*dest = value;

	return 0;
}

static
strp ugh_variable_map_handle(ugh_client_t *c, const char *name, size_t size, void *data)
{
	ugh_module_map_conf_t *conf = data;
	strp vv = ugh_get_varvalue(c, conf->key.data, conf->key.size);

	void **dest;

	dest = JudyLGet(conf->hash, aux_hash_key(vv->data, vv->size), PJE0);
	if (PJERR == dest || NULL == dest) return &conf->default_value;

	return *dest;
}

static
int ugh_command_map(ugh_config_t *cfg, int argc, char **argv)
{
/* msg("map %s -> %s", argv[1], argv[2]); */

	ugh_module_map_conf_t *conf;

	conf = aux_pool_malloc(cfg->pool, sizeof(*conf));
	if (NULL == conf) return -1;

	conf->hash = NULL;

	conf->key.data = argv[1] + 1;
	conf->key.size = strlen(argv[1]) - 1;

	conf->default_value.data = NULL;
	conf->default_value.size = 0;

	ugh_module_handle_add(NULL, conf);

	/* variable */

	ugh_variable_t *v = aux_pool_malloc(cfg->pool, sizeof(ugh_variable_t));
	if (NULL == v) return -1;

	v->name.data = "";
	v->name.size = 0;
	v->handle = ugh_variable_map_handle;
	v->data = conf;

	ugh_idx_variable(cfg, argv[1] + 1, strlen(argv[1]) - 1);
	ugh_set_variable(cfg, argv[2] + 1, strlen(argv[2]) - 1, v);

	ugh_config_parser(cfg, ugh_command_map_handle_line);

	return 0;
}

static ugh_command_t ugh_module_map_cmds [] = {
	ugh_make_command(map),
	ugh_null_command
};

ugh_module_t ugh_module_map = {
	ugh_module_map_cmds,
	NULL,
	NULL
};

