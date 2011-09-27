#include "ugh.h"

typedef struct
{
	ugh_template_t template;
	unsigned wait:1;
} ugh_module_subreq_conf_t;

int ugh_module_subreq_handle(ugh_client_t *c, void *data, strp body)
{
	ugh_module_subreq_conf_t *conf = data;

	strp tv = ugh_template_execute(&conf->template, c);

	if (conf->wait)
	{
		ugh_subreq_t *r = ugh_subreq_add(c, tv->data, tv->size, NULL, UGH_SUBREQ_WAIT, NULL, 0);
		ugh_subreq_wait(c);

		body->data = r->body.data;
		body->size = r->body.size;
	}
	else
	{
		ugh_subreq_add(c, tv->data, tv->size, NULL, 0, NULL, 0);
	}

	return UGH_HTTP_OK;
}

int ugh_command_subreq(ugh_config_t *cfg, int argc, char **argv)
{
	ugh_module_subreq_conf_t *conf;
	
	conf = aux_pool_malloc(cfg->pool, sizeof(*conf));
	if (NULL == conf) return -1;

	ugh_template_compile(&conf->template, argv[1], strlen(argv[1]), cfg);
	conf->wait = (2 < argc && 0 == strcmp(argv[2], "wait"));

	ugh_module_handle_add(ugh_module_subreq_handle, conf);

	return 0;
}

static ugh_command_t ugh_module_subreq_cmds [] =
{
	{ "subreq", ugh_command_subreq },
	{ NULL, NULL }
};

ugh_module_t ugh_module_subreq = 
{
	ugh_module_subreq_cmds,
	NULL,
	NULL
};

