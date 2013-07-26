#ifndef __UGH_MODULE_H__
#define __UGH_MODULE_H__

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ugh_module
	ugh_module_t;

struct ugh_module
{
	ugh_command_t *cmds;

	int (*init)(ugh_config_t *cfg, void *data); /* data is module conf */
	int (*free)(ugh_config_t *cfg, void *data); /* data is module conf */
};

#define UGH_MODULES_MAX 16

extern ugh_module_t *ugh_modules [UGH_MODULES_MAX];
extern size_t ugh_modules_size;

int ugh_module_add(ugh_module_t *m);

#ifdef __cplusplus
}
#endif

#endif /* __UGH_MODULE_H__ */
