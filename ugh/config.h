#ifndef __UGH_CONFIG_H__
#define __UGH_CONFIG_H__

#include <Judy.h>
#include "aux/memory.h"
#include "aux/hashes.h"
#include "aux/system.h"
#include "aux/string.h"

#define UGH_CONFIG_LISTEN_BACKLOG 2048 /* TODO move to config */
#define UGH_CONFIG_SOCKET_TIMEOUT 10.0 /* TODO move to config */
#define UGH_CONFIG_SUBREQ_TIMEOUT 1.0
#define UGH_CONFIG_SILENT_TIMEOUT 1.0 /* TODO move to config */
#define UGH_CONFIG_RESOLVER_TIMEOUT 1.0 /* TODO move to config */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ugh_config
	ugh_config_t;

struct ugh_config
{
	aux_pool_t *pool;

	void *vars_hash; /* "var_name" -> var */
	void *upstreams_hash; /* "upstream_name" -> upstream */

	uint32_t next_upstream; /* TODO one per subrequest */

	char *beg;
	char *pos;
	char *end;

	const char *log_error;
	const char *log_level;

	const char *listen_host;
	unsigned int listen_port;

	const char *resolver_host;
};

int ugh_config_load(ugh_config_t *cfg, const char *filename);
int ugh_config_data(ugh_config_t *cfg, char *data, size_t size);
int ugh_config_free(ugh_config_t *cfg);

typedef int (*ugh_config_handle_fp)(ugh_config_t *, int, char **);
int ugh_config_parser(ugh_config_t *cfg, ugh_config_handle_fp handle);
int ugh_config_option(ugh_config_t *cfg, int argc, char **argv);

/* ### ugh_command ### */

typedef struct ugh_command
	ugh_command_t;

struct ugh_command
{
	const char *name;
	int (*handle)(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd);

	off_t offset;
};

ugh_command_t *ugh_command_get(ugh_config_t *cfg, const char *name);

#define ugh_make_command(name) { #name, ugh_command_##name, 0 }
#define ugh_null_command { NULL, NULL, 0 }

int ugh_config_set_uint_slot(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd);
int ugh_config_set_flag_slot(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd);
int ugh_config_set_time_slot(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd);
int ugh_config_set_char_slot(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd);
int ugh_config_set_strt_slot(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd);
int ugh_config_set_template_slot(ugh_config_t *cfg, int argc, char **argv, ugh_command_t *cmd);

#define ugh_make_command_uint(name, type, number) { #name, ugh_config_set_uint_slot, offsetof(type, member) }
#define ugh_make_command_flag(name, type, member) { #name, ugh_config_set_flag_slot, offsetof(type, member) }
#define ugh_make_command_time(name, type, member) { #name, ugh_config_set_time_slot, offsetof(type, member) }
#define ugh_make_command_char(name, type, member) { #name, ugh_config_set_char_slot, offsetof(type, member) }
#define ugh_make_command_strt(name, type, member) { #name, ugh_config_set_strt_slot, offsetof(type, member) }
#define ugh_make_command_template(name, type, member) { #name, ugh_config_set_template_slot, offsetof(type, member) }

#ifdef __cplusplus
}
#endif

#endif /* __UGH_CONFIG_H__ */
