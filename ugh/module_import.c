#include <dlfcn.h>
#include "ugh.h"

/* TODO dlclose */

static
int ugh_command_import(ugh_config_t *cfg, int argc, char **argv)
{
	char name [1024];
	char pbuf [PATH_MAX], *path;

	snprintf(name, 1024, "ugh_module_%s", argv[1]);

	if (2 < argc)
	{
		path = argv[2];
	}
	else
	{
		snprintf(pbuf, PATH_MAX, UGH_MODULE_PREFIX "%s" UGH_MODULE_SUFFIX, argv[1]);
		path = pbuf;
	}

	void *handle = dlopen(path, RTLD_NOW);

	if (NULL == handle)
	{
		log_emerg("dlopen(%s, RTLD_NOW): %s", path, dlerror());
		return -1;
	}

	char *dl_err = dlerror(); /* clear error before calling dlsym() */

	ugh_module_t *module = (ugh_module_t *) dlsym(handle, name);

	if (NULL != (dl_err = dlerror()))
	{
		log_emerg("dl_err = %s", dl_err);
		return -1;
	}

	ugh_module_add(module);

	return 0;
}

static ugh_command_t ugh_module_import_cmds [] =
{
	{ "import", ugh_command_import },
	{ NULL, NULL }
};

ugh_module_t ugh_module_import =
{
	ugh_module_import_cmds,
	NULL,
	NULL
};

