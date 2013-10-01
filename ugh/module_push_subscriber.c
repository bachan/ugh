#include "ugh.h"

typedef struct
{
	ugh_template_t channel_id;
	unsigned type:1;

#if 1 /* XXX temporary */
	strt location;
#endif

} ugh_module_push_subscriber_conf_t;

extern ugh_module_t ugh_module_push_subscriber;

static
int ugh_module_push_subscriber_handle(ugh_client_t *c, void *data, strp body)
{
	ugh_module_push_subscriber_conf_t *conf = data;

#if 1 /* XXX temporary */
	if (conf->location.size != c->uri.size || 0 != strncmp(conf->location.data, c->uri.data, c->uri.size))
	{
		return UGH_AGAIN;
	}
#endif

	strp channel_id = ugh_template_execute(&conf->channel_id, c);

	if (0 == channel_id->size)
	{
		return UGH_HTTP_BAD_REQUEST;
	}

	ugh_channel_t *ch = ugh_channel_get(c->s, channel_id);

	if (NULL == ch)
	{
		return UGH_HTTP_GONE;
	}

	ugh_header_t *h_if_none_match = ugh_client_header_get_nt(c, "If-None-Match");
	Word_t etag = strtoul(h_if_none_match->value.data, NULL, 10);

	ugh_channel_message_t *m;

	int rc = ugh_channel_get_message(ch, c, &m, conf->type, &etag);

	if (UGH_ERROR == rc)
	{
		return UGH_HTTP_GONE;
	}

	if (UGH_AGAIN == rc)
	{
		return UGH_HTTP_NOT_MODIFIED;
	}

	body->data = aux_pool_strdup(c->pool, &m->body);
	body->size = m->body.size;

	char *content_type_data = aux_pool_strdup(c->pool, &m->content_type);
	ugh_client_header_out_set(c, "Content-Type", sizeof("Content-Type") - 1, content_type_data, m->content_type.size);

	char *etag_data = aux_pool_nalloc(c->pool, 21);
	int etag_size = snprintf(etag_data, 21, "%lu", etag);
	ugh_client_header_out_set(c, "Etag", sizeof("Etag") - 1, etag_data, etag_size);

	return UGH_HTTP_OK;
}

static
int ugh_command_push_subscriber(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd)
{
	ugh_module_push_subscriber_conf_t *conf;

	conf = aux_pool_malloc(cfg->pool, sizeof(*conf));
	if (NULL == conf) return -1;

	ugh_template_compile(&conf->channel_id, argv[1], strlen(argv[1]), cfg);

	if (argc > 2)
	{
		switch (argv[2][0])
		{
		case 'l':
		case 'L':
			conf->type = UGH_CHANNEL_LONG_POLL;
			break;
		case 'i':
		case 'I':
			conf->type = UGH_CHANNEL_INTERVAL_POLL;
			break;
		default:
			return -1;
		}
	}

	ugh_module_handle_add(ugh_module_push_subscriber, conf, ugh_module_push_subscriber_handle);

	return 0;
}

static ugh_command_t ugh_module_push_subscriber_cmds [] =
{
	ugh_make_command(push_subscriber),
	ugh_make_command_strt(push_subscriber_location, ugh_module_push_subscriber_conf_t, location),
	ugh_null_command
};

ugh_module_t ugh_module_push_subscriber =
{
	ugh_module_push_subscriber_cmds,
	NULL,
	NULL
};

