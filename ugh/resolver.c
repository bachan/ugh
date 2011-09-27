#include <stddef.h>
#include "aux/resolver.h"
#include "resolver.h"

static
void ugh_resolver_rec_call_waiters(ugh_resolver_rec_t *rec, in_addr_t addr)
{
	Word_t idx = 0;
	int rc;

	for (rc = Judy1First(rec->wait_hash, &idx, PJE0); 0 != rc;
		 rc = Judy1Next (rec->wait_hash, &idx, PJE0))
	{
		ugh_resolver_ctx_t *ctx = (ugh_resolver_ctx_t *) idx;
		ctx->handle(ctx->data, addr);
	}

	Judy1FreeArray(&rec->wait_hash, PJE0);
}

static
void ugh_resolver_wcb_send(EV_P_ ev_io *w, int tev)
{
	ugh_resolver_rec_t *rec = aux_memberof(ugh_resolver_rec_t, wev_send, w);

	int rc, qlen;
	char q [1024];

	if (0 > (qlen = create_name_query(q, rec->name.data, rec->name.size)))
	{
		ugh_resolver_rec_call_waiters(rec, INADDR_NONE);
		ev_io_stop(loop, &rec->wev_send);
		ev_timer_stop(loop, &rec->wev_timeout);
		return;
	}

	if (0 > (rc = send(w->fd, q, qlen, 0)))
	{
		ugh_resolver_rec_call_waiters(rec, INADDR_NONE);
		ev_io_stop(loop, &rec->wev_send);
		ev_timer_stop(loop, &rec->wev_timeout);
		return;
	}

	ev_io_stop(loop, w);
}

static
void ugh_resolver_wcb_timeout(EV_P_ ev_timer *w, int tev)
{
	ugh_resolver_rec_t *rec = aux_memberof(ugh_resolver_rec_t, wev_timeout, w);
	ugh_resolver_rec_call_waiters(rec, INADDR_NONE);

	ev_io_stop(loop, &rec->wev_send);
	ev_timer_stop(loop, &rec->wev_timeout);
}

static
void ugh_resolver_wcb_recv(EV_P_ ev_io *w, int tev)
{
	ugh_resolver_t *r = aux_memberof(ugh_resolver_t, wev_recv, w);

	char buf [1024];
	int len;

	len = aux_unix_recv(w->fd, buf, 1024);
	if (0 > len) return;

	in_addr_t addrs [8];
	int naddrs;

	strt name;
	char name_data [1024];
	name.data = name_data;

	naddrs = process_response(buf, len, addrs, &name);

	void **dest;

	dest = JudyLGet(r->name_hash, aux_hash_key(name.data, name.size), PJE0);
	if (PJERR == dest || NULL == dest) return;

	ugh_resolver_rec_t *rec = *dest;

	int i;

	rec->naddrs = naddrs;

	for (i = 0; i < naddrs; ++i)
	{
		rec->addrs[i] = addrs[i];
	}

	ugh_resolver_rec_call_waiters(rec, (0 < naddrs) ? addrs[0] : INADDR_NONE);
	ev_timer_stop(loop, &rec->wev_timeout);
}

int ugh_resolver_init(ugh_resolver_t *r, ugh_config_t *cfg)
{
	int sd, rc;

#if 1 /* TODO move ugh_resolver_init operation to config_init callback */
	if (NULL == cfg->resolver_host)
	{
		strt resolv_conf;

		rc = aux_mmap_file(&resolv_conf, "/etc/resolv.conf");
		if (0 > rc) return -1;

		/* TODO use normal resolv.conf parser (which reads comments) */
		char *ns_b = aux_strnstr(resolv_conf.data, "nameserver ", resolv_conf.size);

		if (NULL == ns_b)
		{
			aux_umap(&resolv_conf);
			return -1;
		}

		ns_b += sizeof("nameserver ") - 1;

		char *ns_e = memchr(ns_b, '\n', resolv_conf.data + resolv_conf.size - ns_b);

		char *ns = aux_pool_malloc(cfg->pool, ns_e - ns_b + 1);

		if (NULL == ns)
		{
			aux_umap(&resolv_conf);
			return -1;
		}

		memcpy(ns, ns_b, ns_e - ns_b);
		ns[ns_e - ns_b] = 0;

		cfg->resolver_host = ns;

		rc = aux_umap(&resolv_conf);
		if (0 > rc) return -1;
	}
#endif

	r->pool = aux_pool_init(0);
	if (NULL == r->pool) return -1;

	if (0 > (sd = socket(AF_INET, SOCK_DGRAM, 0)))
	{
		log_error("socket(AF_INET, SOCK_DGRAM, 0) (%d: %s)", errno, aux_strerror(errno));
		return -1;
	}

	if (0 > (rc = aux_set_nonblk(sd, 1)))
	{
		log_error("aux_set_nonblk(%d, 1) (%d: %s)", sd, errno, aux_strerror(errno));
		close(sd);
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(cfg->resolver_host);
	addr.sin_port = htons(53);

	rc = connect(sd, (struct sockaddr *) &addr, sizeof(addr));

	if (0 > rc)
	{
		log_error("connect(%d, ...) (%d: %s)", sd, errno, aux_strerror(errno));
		close(sd);
		return -1;
	}

	ev_io_init(&r->wev_recv, ugh_resolver_wcb_recv, sd, EV_READ);
	ev_io_start(loop, &r->wev_recv);

	return 0;
}

int ugh_resolver_free(ugh_resolver_t *r)
{
	/* TODO free wait array for each resolver_rec */

	JudyLFreeArray(&r->name_hash, PJE0);

	ev_io_stop(loop, &r->wev_recv);
	close(r->wev_recv.fd);

	aux_pool_free(r->pool);

	return 0;
}

#if 0
	1. try inet_addr
	2. ins rec[name], if rec[name] exists and not expired (call ctx->handle and return)
	                  if rec[name] exists and expired (add query, call ctx->handle and return)
					  if rec[name] exists and waiting (add ctx to waiters and return)
					  if rec[name] didnot exist (add query, add ctx to waiters and return)
#endif /*  */

int ugh_resolver_addq(ugh_resolver_t *r, char *name, size_t size, ugh_resolver_ctx_t *ctx)
{
	in_addr_t addr;

	if (INADDR_NONE != (addr = aux_inet_addr(name, size)))
	{
		return ctx->handle(ctx->data, addr);
	}

	void **dest;

	dest = JudyLIns(&r->name_hash, aux_hash_key(name, size), PJE0);
	if (PJERR == dest) return ctx->handle(ctx->data, INADDR_NONE); /* emulate couldn't connect error' */

	ugh_resolver_rec_t *rec = *dest;

	if (NULL != rec)
	{
		if (0 < rec->naddrs) /* result is not empty */
		{
			return ctx->handle(ctx->data, rec->addrs[0]); /* TODO round-robin through naddrs */
		}
		else if (NULL != rec->wait_hash) /* result is empty, but someone is already waiting for result */
		{
			Judy1Set(&rec->wait_hash, (uintptr_t) ctx, PJE0); /* add wait */
		}
		else /* result is empty and noone waiting for it (which means, that there was a resolve error) */
		{
			return ctx->handle(ctx->data, INADDR_NONE);
		}
	}
	else
	{
		rec = (ugh_resolver_rec_t *) aux_pool_calloc(r->pool, sizeof(*rec));
		if (NULL == rec) return -1;

		rec->name.data = (char *) aux_pool_malloc(r->pool, size);
		if (NULL == rec->name.data) return -1;

		rec->name.size = aux_cpymsz(rec->name.data, name, size);

		ev_io_init(&rec->wev_send, ugh_resolver_wcb_send, r->wev_recv.fd, EV_WRITE);
		ev_timer_init(&rec->wev_timeout, ugh_resolver_wcb_timeout, 0, UGH_CONFIG_RESOLVER_TIMEOUT);
		ev_timer_again(loop, &rec->wev_timeout);
		ev_io_start(loop, &rec->wev_send);

		*dest = rec;

		Judy1Set(&rec->wait_hash, (Word_t) ctx, PJE0); /* add wait */
	}

	return 0;
}

