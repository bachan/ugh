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

static
int ugh_command_resolver_timeout(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd)
{
	char *p;

	cfg->resolver_timeout = (double) strtoul(argv[1], &p, 10);

	switch (*p)
	{
	case 'm': cfg->resolver_timeout /= 1000; break;
	case 'u': cfg->resolver_timeout /= 1000000; break;
	case 'n': cfg->resolver_timeout /= 1000000000; break;
	}

	return 0;
}

static
int ugh_command_resolver_cache(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd)
{
	cfg->resolver_cache = (0 == strcmp("on", argv[1]) ? 1 : 0);

	return 0;
}

#if 0
static
int ugh_command_worker_threads(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd)
{
	cfg->worker_threads = strtoul(argv[1], NULL, 10);

	return 0;
}
#endif

static ugh_command_t ugh_module_core_cmds [] =
{
	ugh_make_command(error_log),
	ugh_make_command(listen),
	ugh_make_command(resolver),
	ugh_make_command(resolver_timeout),
	ugh_make_command(resolver_cache),
	/* ugh_make_command(worker_threads), */
	ugh_null_command
};

ugh_module_t ugh_module_core =
{
	ugh_module_core_cmds,
	NULL,
	NULL
};

