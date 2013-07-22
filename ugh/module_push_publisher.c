#include "ugh.h"

/*
 * TODO: Message storage limits SHOULD be configurable. publisher locations
 * SHOULD be configurable to allow foregoing message storage on POST
 * requests.
 *
 *
 * TODO: All 200-level responses MUST, in the response body, contain
 * information about the applicable channel. This information MAY contain
 * the number of stored messages and the number of subscribers' requests
 * being long-held prior to this request. The server MAY implement a
 * content-negotiation scheme for this information.
 */

typedef struct
{
	ugh_template_t channel_id;

#if 1 /* XXX temporary */
	strt location;
#endif

} ugh_module_push_publisher_conf_t;

static
int ugh_module_push_publisher_handle(ugh_client_t *c, void *data, strp body)
{
	ugh_module_push_publisher_conf_t *conf = data;

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

	if (c->method == UGH_HTTP_GET || c->method == UGH_HTTP_HEAD)
	{
		ugh_channel_t *ch = ugh_channel_get(c->s, channel_id);

		if (NULL == ch)
		{
			return UGH_HTTP_NOT_FOUND;
		}

		return UGH_HTTP_OK;
	}

	if (c->method == UGH_HTTP_PUT)
	{
		ugh_channel_t *ch = ugh_channel_add(c->s, channel_id, UGH_CHANNEL_PERMANENT, 0);

		if (NULL == ch)
		{
			return UGH_HTTP_INTERNAL_SERVER_ERROR;
		}

		return UGH_HTTP_OK;
	}

	if (c->method == UGH_HTTP_DELETE)
	{
		int rc = ugh_channel_del(c->s, channel_id);

		if (0 > rc)
		{
			return UGH_HTTP_NOT_FOUND;
		}

		return UGH_HTTP_OK;
	}

	if (c->method == UGH_HTTP_POST)
	{
		ugh_channel_t *ch = ugh_channel_get(c->s, channel_id);

		if (NULL == ch)
		{
			return UGH_HTTP_NOT_FOUND;
		}

		ugh_header_t *h_content_type = ugh_client_header_get_nt(c, "Content-Type");
		ugh_channel_add_message(ch, &c->body, &h_content_type->value, NULL);

		return ch->clients_size ? UGH_HTTP_CREATED : UGH_HTTP_ACCEPTED;
	}

	return UGH_HTTP_BAD_REQUEST;
}

static
int ugh_command_push_publisher(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd)
{
	ugh_module_push_publisher_conf_t *conf;

	conf = aux_pool_malloc(cfg->pool, sizeof(*conf));
	if (NULL == conf) return -1;

	ugh_template_compile(&conf->channel_id, argv[1], strlen(argv[1]), cfg);

	ugh_module_handle_add(ugh_module_push_publisher_handle, conf);

	return 0;
}

static ugh_command_t ugh_module_push_publisher_cmds [] =
{
	ugh_make_command(push_publisher),
	ugh_make_command_strt(push_publisher_location, ugh_module_push_publisher_conf_t, location),
	ugh_null_command
};

ugh_module_t ugh_module_push_publisher =
{
	ugh_module_push_publisher_cmds,
	NULL,
	NULL
};

