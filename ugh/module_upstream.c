#include "ugh.h"

static
int ugh_command_upstream_handle_line(ugh_config_t *cfg, int argc, char **argv)
{
	ugh_upstream_t *upstream = ugh_module_config_get_last();

	if (0 == strcmp(argv[0], "server"))
	{
		if (upstream->values_size < UGH_MAX_UPSTREAM_ELEMENTS)
		{
			ugh_url_t u;
			ugh_parser_url(&u, argv[1], strlen(argv[1]));

			upstream->values[upstream->values_size].host.data = u.host.data;
			upstream->values[upstream->values_size].host.size = u.host.size;
			upstream->values[upstream->values_size].port = strtoul(u.port.data, NULL, 10);

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
			upstream->backup_values[upstream->backup_values_size].port = strtoul(u.port.data, NULL, 10);

			upstream->backup_values_size++;
			upstream->backup_values_curr = upstream->backup_values_size - 1;
		}
	}

	return 0;
}

static
int ugh_command_upstream(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd)
{
	ugh_upstream_t *upstream;

	upstream = ugh_upstream_add(cfg, argv[1], strlen(argv[1]));

	ugh_module_handle_add(NULL, upstream);

	ugh_config_parser(cfg, ugh_command_upstream_handle_line);

	return 0;
}

static ugh_command_t ugh_module_upstream_cmds [] = {
	ugh_make_command(upstream),
	ugh_null_command
};

ugh_module_t ugh_module_upstream = {
	ugh_module_upstream_cmds,
	NULL,
	NULL
};

