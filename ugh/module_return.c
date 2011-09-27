#include "ugh.h"

typedef struct
{
	ugh_template_t template;
} ugh_module_return_conf_t;

int ugh_module_return_handle(ugh_client_t *c, void *data, strp body)
{
	ugh_module_return_conf_t *conf = (ugh_module_return_conf_t *) data;

	strp tv = ugh_template_execute(&conf->template, c);

	body->size = tv->size;
	body->data = tv->data;

	return UGH_HTTP_OK;
}

int ugh_command_return(ugh_config_t *cfg, int argc, char **argv)
{
	ugh_module_return_conf_t *conf;

	conf = aux_pool_malloc(cfg->pool, sizeof(*conf));
	if (NULL == conf) return -1;

	ugh_template_compile(&conf->template, argv[1], strlen(argv[1]), cfg);

	ugh_module_handle_add(ugh_module_return_handle, conf);

	return 0;
}

static ugh_command_t ugh_module_return_cmds [] =
{
	ugh_make_command(return),
	ugh_null_command
};

ugh_module_t ugh_module_return = 
{
	ugh_module_return_cmds,
	NULL,
	NULL
};

