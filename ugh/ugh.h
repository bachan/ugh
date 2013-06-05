#ifndef __UGH_H__
#define __UGH_H__

#include <ev.h>
#include <Judy.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include "aux/hashes.h"
#include "aux/daemon.h"
#include "aux/logger.h"
#include "aux/memory.h"
#include "aux/socket.h"
#include "aux/string.h"
#if 0
#ifndef CORO_UCONTEXT
#define CORO_UCONTEXT 1
#endif
#include "coro/coro.h"
#endif
#include "coro_ucontext/coro_ucontext.h"
#include <ugh/autoconf.h>
#include "config.h"
#include "module.h"
#include "resolver.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ### ugh_server ########################################################## */

typedef struct ugh_server
	ugh_server_t;

struct ugh_server
{
	ugh_config_t *cfg;
	ugh_resolver_t *resolver;
	struct sockaddr_in addr;
	ev_io wev_accept;
	void *channels_hash; /* JudyL (channel_id -> ugh_channel_t *) */
};

int ugh_server_listen(ugh_server_t *s, ugh_config_t *cfg, ugh_resolver_t *resolver);
int ugh_server_enough(ugh_server_t *s);

/* ### ugh_daemon ########################################################## */

typedef struct ugh_daemon
	ugh_daemon_t;

struct ugh_daemon
{
	ugh_config_t cfg;
	ugh_server_t srv;
	ugh_resolver_t resolver;

	ev_timer wev_silent;
};

int ugh_daemon_exec(const char *cfg_filename, unsigned daemon);

extern struct ev_loop *loop;

/* ### ugh_client ########################################################## */

#define UGH_OK 0
#define UGH_ERROR -1
#define UGH_AGAIN -2

#define UGH_HTTP_GET    0
#define UGH_HTTP_HEAD   1
#define UGH_HTTP_POST   2
#define UGH_HTTP_PUT    3
#define UGH_HTTP_DELETE 4
#define UGH_HTTP_MAX    5

#define UGH_HTTP_VERSION_0_9 0
#define UGH_HTTP_VERSION_1_0 1
#define UGH_HTTP_VERSION_1_1 2
#define UGH_HTTP_VERSION_MAX 3

extern const char *ugh_method_string [UGH_HTTP_MAX];
extern const char *ugh_version_string [UGH_HTTP_VERSION_MAX];

#define UGH_HEADER_BUF (32768) /* XXX this is temporary upper header buffer size limit to simplify things */
#define UGH_SUBREQ_BUF (32768 * 4) /* XXX make this dynamically allocated also */
#define UGH_CORO_STACK (32768 * 16)

typedef struct ugh_client
	ugh_client_t;

struct ugh_client
{
	ugh_server_t *s;
	struct sockaddr_in addr;

	ev_io wev_recv;
	ev_io wev_send;
	ev_timer wev_timeout;

	aux_pool_t *pool;

	strt buf_recv; /* UGH_HEADER_BUF */
	strt buf_send;

	/* request */

	char*key_b;
	char*key_e;
	char*val_b;
	char*val_e;

	char*request_beg;
	char*headers_beg;
	char*request_end;

	ucht state;
	ucht method;
	ucht version;

	strt uri;
	strt args;
	void*args_hash;
	void*headers_hash;
#if 1
	void*vars_hash;
#endif

	strt body;
	size_t content_length;

	/* response */

	void*headers_out_hash;
	strp bufs;
	size_t bufs_sumlen;

	struct iovec *iov;
	size_t iov_num;

#if 1
	char *stack;
	coro_context ctx;
#endif

	unsigned wait; /* num of subrequests to wait for */
};

int ugh_client_add(ugh_server_t *s, int sd, struct sockaddr_in *addr);
int ugh_client_del(ugh_client_t *c);

int ugh_client_send(ugh_client_t *c, int status);

strp ugh_client_getarg_nt(ugh_client_t *c, const char *data);
strp ugh_client_getarg(ugh_client_t *c, const char *data, size_t size);
strp ugh_client_setarg(ugh_client_t *c, const char *data, size_t size, char *value_data, size_t value_size);

typedef struct ugh_header
	ugh_header_t;

struct ugh_header
{
	strt key;
	strt value;
};

ugh_header_t *ugh_client_header_get_nt(ugh_client_t *c, const char *data);
ugh_header_t *ugh_client_header_get(ugh_client_t *c, const char *data, size_t size);
ugh_header_t *ugh_client_header_set(ugh_client_t *c, const char *data, size_t size, char *value_data, size_t value_size);

ugh_header_t *ugh_client_header_out_set(ugh_client_t *c, const char *data, size_t size, char *value_data, size_t value_size);

#define ugh_client_header_out_set_nt(c, data, value_data) ugh_client_header_out_set((c), (data), strlen(data), (char *) (value_data), strlen(value_data))

#if 1
strp ugh_client_getvar_nt(ugh_client_t *c, const char *data);
strp ugh_client_getvar(ugh_client_t *c, const char *data, size_t size);
strp ugh_client_setvar(ugh_client_t *c, const char *data, size_t size, char *value_data, size_t value_size);
#endif

strp ugh_client_cookie_get(ugh_client_t *c, const char *name, size_t size);

/* ### url ### */

typedef struct ugh_url
	ugh_url_t;

struct ugh_url
{
	strt host;
	strt port;
	strt uri;
	strt args;
};

int ugh_parser_url(ugh_url_t *u, char *data, size_t size);

/* ### upstream ### */

#define UGH_MAX_UPSTREAM_ELEMENTS 1024

typedef struct ugh_upstream_server
	ugh_upstream_server_t;

struct ugh_upstream_server
{
	strt host;
	in_port_t port;
};

typedef struct ugh_upstream
	ugh_upstream_t;

struct ugh_upstream
{
	ugh_upstream_server_t values [UGH_MAX_UPSTREAM_ELEMENTS];
	size_t values_size;
	size_t values_curr;

	ugh_upstream_server_t backup_values [UGH_MAX_UPSTREAM_ELEMENTS];
	size_t backup_values_size;
	size_t backup_values_curr;
};

ugh_upstream_t *ugh_upstream_add(ugh_config_t *cfg, const char *name, size_t size);
ugh_upstream_t *ugh_upstream_get(ugh_config_t *cfg, const char *name, size_t size);

/* ### subreq ### */

typedef struct ugh_subreq
	ugh_subreq_t;

typedef int (*ugh_subreq_handle_fp)(ugh_subreq_t *r, char *data, size_t size);

#define UGH_TIMEOUT_ONCE 0
#define UGH_TIMEOUT_FULL 1

struct ugh_subreq
{
	ugh_client_t *c;
	struct sockaddr_in addr;

	ev_io wev_recv;
	ev_io wev_send;
	ev_io wev_connect;
	ev_timer wev_timeout;
	ev_tstamp timeout;
	int timeout_type;

	/* send */

	ugh_url_t u;
	strt request_body;

	ucht method;

	/* void*headers_out_hash; */

	char*buf_send_data;
	strt buf_send; /* UGH_SUBREQ_BUF */

	/* recv */

	char*key_b;
	char*key_e;
	char*val_b;
	char*val_e;

	char*request_beg;
	char*headers_beg;
	char*request_end;

	ucht state;
	ucht version;
	unsigned int status;

	void*headers_hash;

	char*buf_recv_data;
	strt buf_recv; /* UGH_SUBREQ_BUF */
	strt body;

	size_t content_length; /* if content_length is -1, then transfer-encoding is chunked */

	ev_tstamp response_time;
	uint32_t ft_type;

	/* chunks */

	char*chunk_start;
	size_t chunk_size;
	size_t chunk_body_size;

	/* done */

	int flags;
	ugh_subreq_handle_fp handle;

	/* tmp */

	ugh_resolver_ctx_t *resolver_ctx;

	/* tmp */

	ugh_upstream_t *upstream;
	unsigned int upstream_current;
	unsigned int upstream_tries;
};

#define UGH_SUBREQ_WAIT 1

ugh_subreq_t *ugh_subreq_add(ugh_client_t *c, char *url, size_t size, int flags);
int ugh_subreq_set_method(ugh_subreq_t *r, ucht method);
int ugh_subreq_set_header(ugh_subreq_t *r, char *key, size_t key_size, char *value, size_t value_size);
int ugh_subreq_set_body(ugh_subreq_t *r, char *body, size_t body_size);
int ugh_subreq_set_timeout(ugh_subreq_t *r, ev_tstamp timeout, int timeout_type);
int ugh_subreq_run(ugh_subreq_t *r);
int ugh_subreq_gen(ugh_subreq_t *r, strp u_host);
int ugh_subreq_del(ugh_subreq_t *r, uint32_t ft_type);

ugh_upstream_server_t *ugh_subreq_get_upstream_curr(ugh_subreq_t *r); /* deprecated */

strp ugh_subreq_get_host(ugh_subreq_t *r);
in_port_t ugh_subreq_get_port(ugh_subreq_t *r);

#define UGH_UPSTREAM_FT_OFF            0x0000
#define UGH_UPSTREAM_FT_ERROR          0x0001
#define UGH_UPSTREAM_FT_TIMEOUT        0x0002
#define UGH_UPSTREAM_FT_INVALID_HEADER 0x0004
#define UGH_UPSTREAM_FT_HTTP_500       0x0008
#define UGH_UPSTREAM_FT_HTTP_502       0x0010
#define UGH_UPSTREAM_FT_HTTP_503       0x0020
#define UGH_UPSTREAM_FT_HTTP_504       0x0040
#define UGH_UPSTREAM_FT_HTTP_404       0x0080
#define UGH_UPSTREAM_FT_HTTP_5XX       0x0100
#define UGH_UPSTREAM_FT_HTTP_4XX       0x0200

void ugh_subreq_wait(ugh_client_t *c);

ugh_header_t *ugh_subreq_header_get_nt(ugh_subreq_t *r, const char *data);
ugh_header_t *ugh_subreq_header_get(ugh_subreq_t *r, const char *data, size_t size);
ugh_header_t *ugh_subreq_header_set(ugh_subreq_t *r, const char *data, size_t size, char *value_data, size_t value_size);

/* ### parser ### */

int ugh_parser_client(ugh_client_t *c, char *data, size_t size);
int ugh_parser_subreq(ugh_subreq_t *r, char *data, size_t size);
int ugh_parser_chunks(ugh_subreq_t *r, char *data, size_t size);

/* ### channel ### */

#define UGH_CHANNEL_LONG_POLL 0
#define UGH_CHANNEL_INTERVAL_POLL 1

#define UGH_CHANNEL_WORKING 0
#define UGH_CHANNEL_DELETED 1

typedef struct ugh_channel
	ugh_channel_t;

struct ugh_channel
{
	ugh_server_t *s;
	aux_pool_t *pool;

	ev_async wev_message;

	void *messages_hash; /* JudyL (etag -> ugh_channel_message_t *) */
	/* TODO last-modified / if-modified-since mechanism */

	void *clients_hash; /* Judy1 (ugh_client_t *) */ /* XXX Judy is not required here */
	size_t clients_size; /* XXX we can use plain array with this size variable */

	unsigned status:1;
};

ugh_channel_t *ugh_channel_add(ugh_server_t *s, strp channel_id);
ugh_channel_t *ugh_channel_get(ugh_server_t *s, strp channel_id);
int ugh_channel_del(ugh_server_t *s, strp channel_id);

int ugh_channel_add_message(ugh_channel_t *ch, strp body, strp content_type);
int ugh_channel_get_message(ugh_channel_t *ch, ugh_client_t *c, strp body, unsigned type);

typedef struct ugh_channel_message
	ugh_channel_message_t;

struct ugh_channel_message
{
	strt content_type;
	strt body;
};

/* ### status ### */

#define UGH_HTTP_OK                                 0x00 /* 200 */
#define UGH_HTTP_CREATED                            0x01 /* 201 */
#define UGH_HTTP_ACCEPTED                           0x02 /* 202 */
#define UGH_HTTP_NON_AUTHORITATIVE_INFORMATION      0x03 /* 203 */
#define UGH_HTTP_NO_CONTENT                         0x04 /* 204 */
#define UGH_HTTP_RESET_CONTENT                      0x05 /* 205 */
#define UGH_HTTP_PARTIAL_CONTENT                    0x06 /* 206 */
#define UGH_HTTP_MULTI_STATUS                       0x07 /* 207 */
#define UGH_HTTP_MULTIPLE_CHOICES                   0x08 /* 300 */
#define UGH_HTTP_MOVED_PERMANENTLY                  0x09 /* 301 */
#define UGH_HTTP_MOVED_TEMPORARILY                  0x0A /* 302 */
#define UGH_HTTP_SEE_OTHER                          0x0B /* 303 */
#define UGH_HTTP_NOT_MODIFIED                       0x0C /* 304 */
#define UGH_HTTP_USE_PROXY                          0x0D /* 305 */
#define UGH_HTTP_UNUSED                             0x0E /* 306 */
#define UGH_HTTP_TEMPORARY_REDIRECT                 0x0F /* 307 */
#define UGH_HTTP_BAD_REQUEST                        0x10 /* 400 */
#define UGH_HTTP_UNAUTHORIZED                       0x11 /* 401 */
#define UGH_HTTP_PAYMENT_REQUIRED                   0x12 /* 402 */
#define UGH_HTTP_FORBIDDEN                          0x13 /* 403 */
#define UGH_HTTP_NOT_FOUND                          0x14 /* 404 */
#define UGH_HTTP_NOT_ALLOWED                        0x15 /* 405 */
#define UGH_HTTP_NOT_ACCEPTABLE                     0x16 /* 406 */
#define UGH_HTTP_PROXY_AUTHENTICATION_REQUIRED      0x17 /* 407 */
#define UGH_HTTP_REQUEST_TIME_OUT                   0x18 /* 408 */
#define UGH_HTTP_CONFLICT                           0x19 /* 409 */
#define UGH_HTTP_GONE                               0x1A /* 410 */
#define UGH_HTTP_LENGTH_REQUIRED                    0x1B /* 411 */
#define UGH_HTTP_PRECONDITION_FAILED                0x1C /* 412 */
#define UGH_HTTP_REQUEST_ENTITY_TOO_LARGE           0x1D /* 413 */
#define UGH_HTTP_REQUEST_URI_TOO_LARGE              0x1E /* 414 */
#define UGH_HTTP_UNSUPPORTED_MEDIA_TYPE             0x1F /* 415 */
#define UGH_HTTP_REQUESTED_RANGE_NOT_SATISFIABLE    0x20 /* 416 */
#define UGH_HTTP_EXPECTATION_FAILED                 0x21 /* 417 */
#define UGH_HTTP_INTERNAL_SERVER_ERROR              0x22 /* 500 */
#define UGH_HTTP_METHOD_NOT_IMPLEMENTED             0x23 /* 501 */
#define UGH_HTTP_BAD_GATEWAY                        0x24 /* 502 */
#define UGH_HTTP_SERVICE_TEMPORARILY_UNAVAILABLE    0x25 /* 503 */
#define UGH_HTTP_GATEWAY_TIME_OUT                   0x26 /* 504 */
#define UGH_HTTP_HTTP_VERSION_NOT_SUPPORTED         0x27 /* 505 */
#define UGH_HTTP_VARIANT_ALSO_NEGOTIATES            0x28 /* 506 */
#define UGH_HTTP_INSUFFICIENT_STORAGE               0x29 /* 507 */
#define UGH_HTTP_STATUS_MAX                         0x2A

extern const char *ugh_status_header [UGH_HTTP_STATUS_MAX];

/* ### module ### */

#if 1
typedef int (*ugh_module_handle_fp) (ugh_client_t *c, void *data, strp body);

#define	UGH_MODULE_HANDLES_MAX 1024

extern ugh_module_handle_fp ugh_module_handles [UGH_MODULE_HANDLES_MAX];
extern void * ugh_module_configs [UGH_MODULE_HANDLES_MAX];
extern size_t ugh_module_handles_size;

#define ugh_module_handle_add(handle, config) \
	ugh_module_handles[ugh_module_handles_size] = handle; \
	ugh_module_configs[ugh_module_handles_size] = config; \
	ugh_module_handles_size++

int ugh_module_handle(ugh_client_t *c);

#define ugh_module_config_get_last() ugh_module_configs[ugh_module_handles_size - 1]
#endif

#if 1
extern coro_context ctx_main;
extern unsigned char is_main_coro;
#endif

/* ### variables ### */

typedef struct ugh_variable
	ugh_variable_t;

struct ugh_variable
{
	strt name;
	strp (*handle)(ugh_client_t *c, const char *name, size_t size, void *data);
	void *data;
};

void ugh_idx_variable(ugh_config_t *cfg, const char *name, size_t size);
ugh_variable_t *ugh_set_variable(ugh_config_t *cfg, const char *name, size_t size, ugh_variable_t *var);
ugh_variable_t *ugh_get_variable(ugh_config_t *cfg, const char *name, size_t size);

strp ugh_get_varvalue(ugh_client_t *c, const char *name, size_t size);

#define ugh_get_varvalue_nt(c, name) ugh_get_varvalue((c), (name), strlen(name))

/* ### ugh_template ### */

#define UGH_TEMPLATE_MAX_CHUNKS 16
#define UGH_TEMPLATE_MAX_LENGTH 4096

typedef struct ugh_template
	ugh_template_t;

struct ugh_template
{
	strp chunks;
	size_t chunks_size;
};

int  ugh_template_compile(ugh_template_t *t, char *data, size_t size, ugh_config_t *cfg);
strp ugh_template_execute(ugh_template_t *t, ugh_client_t *c);

#ifdef __cplusplus
}
#endif

#endif /* __UGH_H__ */
