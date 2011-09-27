#include "ugh.h"

static
int ugh_command_upstream_handle_line(ugh_config_t *cfg, int argc, char **argv)
{
	ugh_upstream_t *upstream = ugh_module_configs[ugh_module_handles_size - 1]; /* TODO make this API element */

	if (0 == strcmp(argv[0], "server"))
	{
		if (upstream->values_size < UGH_MAX_UPSTREAM_ELEMENTS)
		{
			ugh_url_t u;
			ugh_parser_url(&u, argv[1], strlen(argv[1]));

			upstream->values[upstream->values_size].host.data = u.host.data;
			upstream->values[upstream->values_size].host.size = u.host.size;
			upstream->values[upstream->values_size].port = htons(strtoul(u.port.data, NULL, 10));

			upstream->values_size++;
			upstream->values_curr = upstream->values_size - 1;
		}
	}

	if (0 == strcmp(argv[0], "backup"))
	{
		if (upstream->backup_values_size < UGH_MAX_UPSTREAM_ELEMENTS)
		{
			ugh_url_t u;
			ugh_parser_url(&u, argv[1], strlen(argv[1]));

			upstream->backup_values[upstream->backup_values_size].host.data = u.host.data;
			upstream->backup_values[upstream->backup_values_size].host.size = u.host.size;
			upstream->backup_values[upstream->backup_values_size].port = htons(strtoul(u.port.data, NULL, 10));

			upstream->backup_values_size++;
			upstream->backup_values_curr = upstream->backup_values_size - 1;
		}
	}

	return 0;
}

static
int ugh_command_upstream(ugh_config_t *cfg, int argc, char **argv)
{
	ugh_upstream_t *upstream;

	upstream = ugh_upstream_add(cfg, argv[1], strlen(argv[1]));

	ugh_module_handle_add(NULL, upstream);

	ugh_config_parser(cfg, ugh_command_upstream_handle_line);

	return 0;
}

static
int ugh_command_proxy_next_upstream(ugh_config_t *cfg, int argc, char **argv)
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

static ugh_command_t ugh_module_upstream_cmds [] = {
	ugh_make_command(upstream),
	ugh_make_command(proxy_next_upstream),
	ugh_null_command
};

ugh_module_t ugh_module_upstream = {
	ugh_module_upstream_cmds,
	NULL,
	NULL
};

