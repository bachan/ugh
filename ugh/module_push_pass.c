#include "ugh.h"

typedef struct
{
	ugh_template_t channel_id;
	ugh_template_t url;
	double recv_timeout;

#if 1 /* XXX temporary */
	strt location;
#endif

} ugh_module_push_pass_conf_t;

static
int ugh_module_push_pass_handle(ugh_client_t *c, void *data, strp body)
{
	ugh_module_push_pass_conf_t *conf = data;

#if 1 /* XXX temporary */
	if (conf->location.size != c->uri.size || 0 != strncmp(conf->location.data, c->uri.data, c->uri.size))
	{
		return UGH_AGAIN;
	}
#endif

	/* add channel */

	strp channel_id = ugh_template_execute(&conf->channel_id, c);

	ugh_channel_t *ch = ugh_channel_add(c->s, channel_id, UGH_CHANNEL_PROXY, 0); /* TODO add configurable timeout for channel lifetime instead of 0 here */

	if (NULL == ch)
	{
		return UGH_HTTP_INTERNAL_SERVER_ERROR;
	}

	/* perform subrequest */

	strp url = ugh_template_execute(&conf->url, c);

	ugh_subreq_t *r = ugh_subreq_add(c, url->data, url->size, UGH_SUBREQ_PUSH);
	ugh_subreq_set_channel(r, ch, 0);

	if (conf->recv_timeout > 0) /* XXX to remove this if we should support default values per each block in _init function */
	{
		ugh_subreq_set_timeout(r, conf->recv_timeout, UGH_TIMEOUT_ONCE);
	}

	ugh_subreq_run(r);

	return UGH_HTTP_OK;
}

static
int ugh_command_push_pass(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd)
{
	ugh_module_push_pass_conf_t *conf;

	conf = aux_pool_malloc(cfg->pool, sizeof(*conf));
	if (NULL == conf) return -1;

	ugh_template_compile(&conf->channel_id, argv[1], strlen(argv[1]), cfg);
	ugh_template_compile(&conf->url, argv[2], strlen(argv[2]), cfg);

	ugh_module_handle_add(ugh_module_push_pass_handle, conf);

	return 0;
}

static ugh_command_t ugh_module_push_pass_cmds [] =
{
	ugh_make_command(push_pass),
	ugh_make_command_strt(push_pass_location, ugh_module_push_pass_conf_t, location),
	ugh_make_command_time(push_pass_recv_timeout, ugh_module_push_pass_conf_t, recv_timeout),
	ugh_null_command
};

ugh_module_t ugh_module_push_pass =
{
	ugh_module_push_pass_cmds,
	NULL,
	NULL
};

