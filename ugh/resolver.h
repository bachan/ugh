#ifndef __UGH_RESOLVER_H__
#define __UGH_RESOLVER_H__

#include <ev.h>
#include <Judy.h>
#include <unistd.h>
#include "aux/hashes.h"
#include "aux/logger.h"
#include "aux/memory.h"
#include "aux/socket.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ugh_resolver_ctx
	ugh_resolver_ctx_t;

struct ugh_resolver_ctx
{
	int (*handle)(void *data, in_addr_t addr); /* called when resolve is done (or address is already known) */
	void *data;
};

typedef struct ugh_resolver_rec
	ugh_resolver_rec_t;

struct ugh_resolver_rec
{
	strt name;
	int naddrs;
	in_addr_t addrs [8];

	ev_io wev_send;
	ev_timer wev_timeout;

	void *wait_hash; /* Judy (ugh_resolver_ctx_t *) */
};

typedef struct ugh_resolver
	ugh_resolver_t;

struct ugh_resolver
{
	aux_pool_t *pool;
	ev_io wev_recv;
	void *name_hash; /* Judy (domain -> ugh_resolver_rec_t *) */
};

int ugh_resolver_init(ugh_resolver_t *r, ugh_config_t *cfg);
int ugh_resolver_free(ugh_resolver_t *r);
int ugh_resolver_addq(ugh_resolver_t *r, char *name, size_t size, ugh_resolver_ctx_t *ctx);

#if 1
extern struct ev_loop *loop;
#endif

#ifdef __cplusplus
}
#endif

#endif /* __UGH_RESOLVER_H__ */
