#include "ugh.h"

typedef struct
{
	ugh_template_t url;
	unsigned nowait;
} ugh_module_proxy_conf_t;

int ugh_module_proxy_handle(ugh_client_t *c, void *data, strp body)
{
	ugh_module_proxy_conf_t *conf = data;

	strp tv = ugh_template_execute(&conf->url, c);

	if (conf->nowait)
	{
		ugh_subreq_t *r = ugh_subreq_add(c, tv->data, tv->size, 0);
		ugh_subreq_run(r);
	}
	else
	{
		ugh_subreq_t *r = ugh_subreq_add(c, tv->data, tv->size, UGH_SUBREQ_WAIT);
		ugh_subreq_run(r);
		ugh_subreq_wait(c);

		body->data = r->body.data;
		body->size = r->body.size;
	}

	return UGH_HTTP_OK;
}

int ugh_command_proxy_pass(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd)
{
	ugh_module_proxy_conf_t *conf;
	
	conf = aux_pool_malloc(cfg->pool, sizeof(*conf));
	if (NULL == conf) return -1;

	ugh_template_compile(&conf->url, argv[1], strlen(argv[1]), cfg);

	ugh_module_handle_add(ugh_module_proxy_handle, conf);

	return 0;
}

int ugh_command_proxy_nowait(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd)
{
	ugh_module_proxy_conf_t *conf = ugh_module_config_get_last();

	conf->nowait = (0 == strcmp(argv[1], "on") ? 1 : 0);

	return 0;
}

static
int ugh_command_proxy_next_upstream(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd)
{
	int i;

	for (i = 1; i < argc; ++i)
	{
		if (0 == strcmp(argv[i], "error"))
		{
			cfg->next_upstream |= UGH_UPSTREAM_FT_ERROR;
		}
		else if (0 == strcmp(argv[i], "timeout"))
		{
			cfg->next_upstream |= UGH_UPSTREAM_FT_TIMEOUT;
		}
		else if (0 == strcmp(argv[i], "invalid_header"))
		{
			cfg->next_upstream |= UGH_UPSTREAM_FT_INVALID_HEADER;
		}
		else if (0 == strcmp(argv[i], "http_500"))
		{
			cfg->next_upstream |= UGH_UPSTREAM_FT_HTTP_500;
		}
		else if (0 == strcmp(argv[i], "http_502"))
		{
			cfg->next_upstream |= UGH_UPSTREAM_FT_HTTP_502;
		}
		else if (0 == strcmp(argv[i], "http_503"))
		{
			cfg->next_upstream |= UGH_UPSTREAM_FT_HTTP_503;
		}
		else if (0 == strcmp(argv[i], "http_504"))
		{
			cfg->next_upstream |= UGH_UPSTREAM_FT_HTTP_504;
		}
		else if (0 == strcmp(argv[i], "http_404"))
		{
			cfg->next_upstream |= UGH_UPSTREAM_FT_HTTP_404;
		}
		else if (0 == strcmp(argv[i], "http_5xx"))
		{
			cfg->next_upstream |= UGH_UPSTREAM_FT_HTTP_5XX;
		}
		else if (0 == strcmp(argv[i], "http_4xx"))
		{
			cfg->next_upstream |= UGH_UPSTREAM_FT_HTTP_4XX;
		}
		else if (0 == strcmp(argv[i], "off"))
		{
			cfg->next_upstream  = UGH_UPSTREAM_FT_OFF;
		}
	}

	return 0;
}

static ugh_command_t ugh_module_subreq_cmds [] =
{
	ugh_make_command(proxy_pass),
	ugh_make_command_flag(proxy_nowait, offsetof(ugh_module_proxy_conf_t, nowait)),
	ugh_make_command(proxy_next_upstream),
	ugh_null_command
};

ugh_module_t ugh_module_subreq = 
{
	ugh_module_subreq_cmds,
	NULL,
	NULL
};

