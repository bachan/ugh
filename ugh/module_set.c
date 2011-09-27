#include "ugh.h"

#define UGH_MAX_SET_ELEMENTS 16

typedef struct
{
	strt values [UGH_MAX_SET_ELEMENTS];
	size_t size;
	size_t curr;
} ugh_module_set_conf_t;

static
int ugh_command_set_handle_line(ugh_config_t *cfg, int argc, char **argv)
{
	ugh_module_set_conf_t *conf = ugh_module_configs[ugh_module_handles_size - 1]; /* TODO make this API element */

	if (conf->size < UGH_MAX_SET_ELEMENTS)
	{
		conf->values[conf->size].size = strlen(argv[0]);
		conf->values[conf->size].data = argv[0];

		conf->size++;
		conf->curr = conf->size - 1; /* this is to return first value first */
	}

	return 0;
}

static
strp ugh_variable_set_handle(ugh_client_t *c, const char *name, size_t size, void *data)
{
	ugh_module_set_conf_t *conf = data;

	conf->curr += 1;
	conf->curr %= conf->size;

	return &conf->values[conf->curr];
}

static
int ugh_command_set(ugh_config_t *cfg, int argc, char **argv)
{
	ugh_module_set_conf_t *conf;

	conf = aux_pool_calloc(cfg->pool, sizeof(*conf));
	if (NULL == conf) return -1;

	ugh_module_handle_add(NULL, conf);

	/* variable */

	ugh_variable_t *v;

	v = aux_pool_malloc(cfg->pool, sizeof(*v));
	if (NULL == v) return -1;

	v->name.data = "";
	v->name.size = 0;
	v->handle = ugh_variable_set_handle;
	v->data = conf;

	ugh_set_variable(cfg, argv[1] + 1, strlen(argv[1]) - 1, v);

	ugh_config_parser(cfg, ugh_command_set_handle_line);

	return 0;
}

static ugh_command_t ugh_module_set_cmds [] = {
	ugh_make_command(set),
	ugh_null_command
};

ugh_module_t ugh_module_set = {
	ugh_module_set_cmds,
	NULL,
	NULL
};

