#include "module.h"

extern ugh_module_t ugh_module_core;
extern ugh_module_t ugh_module_import;
extern ugh_module_t ugh_module_map;
extern ugh_module_t ugh_module_set;
extern ugh_module_t ugh_module_return;
extern ugh_module_t ugh_module_subreq;
extern ugh_module_t ugh_module_upstream;

ugh_module_t *ugh_modules [UGH_MODULES_MAX] = {
	&ugh_module_core,
	&ugh_module_import,
	&ugh_module_map,
	&ugh_module_set,
	&ugh_module_return,
	&ugh_module_subreq,
	&ugh_module_upstream,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

size_t ugh_modules_size = 7;

int ugh_module_add(ugh_module_t *m)
{
	if (ugh_modules_size >= UGH_MODULES_MAX)
	{
		return -1;
	}

	ugh_modules[ugh_modules_size++] = m;

	return 0;
}

