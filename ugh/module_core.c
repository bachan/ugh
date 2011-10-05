#include "ugh.h"

static
int ugh_command_error_log(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd)
{
	cfg->log_error = argv[1];
	cfg->log_level = (3 > argc) ? "error" : argv[2];

	return 0;
}

static
int ugh_command_listen(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd)
{
	char *port = strchr(argv[1], ':');

	if (NULL == port)
	{
		cfg->listen_host = "0.0.0.0";
		cfg->listen_port = strtoul(argv[1], NULL, 10);
	}
	else
	{
		*port++ = 0;

		cfg->listen_host = argv[1];
		cfg->listen_port = strtoul(port, NULL, 10);
	}

	return 0;
}

static
int ugh_command_resolver(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd)
{
	cfg->resolver_host = argv[1];

	return 0;
}

static ugh_command_t ugh_module_core_cmds [] =
{
	ugh_make_command(error_log),
	ugh_make_command(listen),
	ugh_make_command(resolver),
	ugh_null_command
};

ugh_module_t ugh_module_core =
{
	ugh_module_core_cmds,
	NULL,
	NULL
};

