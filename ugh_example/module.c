#include <ugh/ugh.h>

typedef struct
{
	ugh_template_t session_host;
	ugh_template_t friends_host;
	ugh_template_t wall_host;
	ugh_template_t logger_host;
} ugh_module_example_conf_t;

int ugh_module_example_init(ugh_config_t *cfg)
{
	log_info("ugh_module_example_init (called each time server is started)");

	return 0;
}

int ugh_module_example_free()
{
	log_info("ugh_module_example_free (called each time server is stopped)");

	return 0;
}

int ugh_module_example_handle(ugh_client_t *c, void *data, strp body)
{
	ugh_module_example_conf_t *conf = data; /* get module conf */

	strp cookie_sid = ugh_client_cookie_get(c, "sid", 3);

	/* order one session subrequest */

	strp session_host = ugh_template_execute(&conf->session_host, c);
	ugh_subreq_t *r_session = ugh_subreq_add(c, session_host->data, session_host->size, NULL, UGH_SUBREQ_WAIT, cookie_sid->data, cookie_sid->size);

	/* wait for it (do smth in different coroutine while it is being downloaded) */

	ugh_subreq_wait(c);

	if (0 == cookie_sid->size)
	{
		ugh_client_header_out_set(c, "Set-Cookie", sizeof("Set-Cookie") - 1, r_session->body.data, r_session->body.size);

		return UGH_HTTP_OK;
	}

	if (0 == r_session->body.size)
	{
		body->data = "Please login [login form]";
		body->size = sizeof("Please login [login form]") - 1;

		return UGH_HTTP_OK;
	}

	/* order friends subrequest */

	strp friends_host = ugh_template_execute(&conf->friends_host, c);
	ugh_subreq_t *r_friends = ugh_subreq_add(c, friends_host->data, friends_host->size, NULL, UGH_SUBREQ_WAIT, r_session->body.data, r_session->body.size);

	/* order wall subrequest */

	strp wall_host = ugh_template_execute(&conf->wall_host, c);
	ugh_subreq_t *r_wall = ugh_subreq_add(c, wall_host->data, wall_host->size, NULL, UGH_SUBREQ_WAIT, r_session->body.data, r_session->body.size);

	/* order logger subrequest, but tell ugh not to wait for its result */

	strp logger_host = ugh_template_execute(&conf->logger_host, c);
	ugh_subreq_add(c, logger_host->data, logger_host->size, NULL, 0, r_session->body.data, r_session->body.size);

	/* wait for two ordered subrequests */

	ugh_subreq_wait(c);

	/* set body */

	body->size = 0;
	body->data = aux_pool_malloc(c->pool, r_friends->body.size + r_wall->body.size);
	if (NULL == body->data) return UGH_HTTP_INTERNAL_SERVER_ERROR;

	body->size += aux_cpymsz(body->data + body->size, r_friends->body.data, r_friends->body.size);
	body->size += aux_cpymsz(body->data + body->size, r_wall->body.data, r_wall->body.size);

	/* return 200 OK status */

	return UGH_HTTP_OK;
}

int ugh_command_example(ugh_config_t *cfg, int argc, char **argv)
{
	ugh_module_example_conf_t *conf;
	
	conf = aux_pool_malloc(cfg->pool, sizeof(*conf));
	if (NULL == conf) return -1;

	ugh_module_handle_add(ugh_module_example_handle, conf);

	return 0;
}

int ugh_command_example_session_host(ugh_config_t *cfg, int argc, char **argv)
{
	ugh_module_example_conf_t *conf = ugh_module_configs[ugh_module_handles_size - 1];
	ugh_template_compile(&conf->session_host, argv[1], strlen(argv[1]), cfg);

	return 0;
}

int ugh_command_example_friends_host(ugh_config_t *cfg, int argc, char **argv)
{
	ugh_module_example_conf_t *conf = ugh_module_configs[ugh_module_handles_size - 1];
	ugh_template_compile(&conf->friends_host, argv[1], strlen(argv[1]), cfg);

	return 0;
}

int ugh_command_example_wall_host(ugh_config_t *cfg, int argc, char **argv)
{
	ugh_module_example_conf_t *conf = ugh_module_configs[ugh_module_handles_size - 1];
	ugh_template_compile(&conf->wall_host, argv[1], strlen(argv[1]), cfg);

	return 0;
}

int ugh_command_example_logger_host(ugh_config_t *cfg, int argc, char **argv)
{
	ugh_module_example_conf_t *conf = ugh_module_configs[ugh_module_handles_size - 1];
	ugh_template_compile(&conf->logger_host, argv[1], strlen(argv[1]), cfg);

	return 0;
}

static ugh_command_t ugh_module_example_cmds [] =
{
	ugh_make_command(example),
	ugh_make_command(example_session_host),
	ugh_make_command(example_friends_host),
	ugh_make_command(example_wall_host),
	ugh_make_command(example_logger_host),
	ugh_null_command
};

ugh_module_t ugh_module_example = 
{
	ugh_module_example_cmds,
	ugh_module_example_init,
	ugh_module_example_free
};

