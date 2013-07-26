#include "ugh.h"

typedef struct
{
	ugh_template_t key;
	ugh_template_t value;
} ugh_module_add_header_conf_t;

static
int ugh_module_add_header_handle(ugh_client_t *c, void *data, strp body)
{
	ugh_module_add_header_conf_t *conf = (ugh_module_add_header_conf_t *) data;

	strp key = ugh_template_execute(&conf->key, c);
	strp value = ugh_template_execute(&conf->value, c);

	ugh_client_header_out_set(c, key->data, key->size, value->data, value->size);

	return UGH_HTTP_OK;
}

static
int ugh_command_add_header(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd)
{
	ugh_module_add_header_conf_t *conf;

	conf = aux_pool_malloc(cfg->pool, sizeof(*conf));
	if (NULL == conf) return -1;

	ugh_template_compile(&conf->key, argv[1], strlen(argv[1]), cfg);
	ugh_template_compile(&conf->value, argv[2], strlen(argv[2]), cfg);

	ugh_module_handle_add(ugh_module_add_header_handle, conf);

	return 0;
}

static ugh_command_t ugh_module_add_header_cmds [] =
{
	ugh_make_command(add_header),
	ugh_null_command
};

ugh_module_t ugh_module_add_header = 
{
	ugh_module_add_header_cmds,
	NULL,
	NULL
};

