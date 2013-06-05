#include "ugh.h"

static
int ugh_command_push_pass(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd)
{
	return 0;
}

static ugh_command_t ugh_module_push_pass_cmds [] =
{
	ugh_make_command(push_pass),
	ugh_null_command
};

ugh_module_t ugh_module_push_pass =
{
	ugh_module_push_pass_cmds,
	NULL,
	NULL
};

