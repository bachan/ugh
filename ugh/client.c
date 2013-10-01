#include "ugh.h"

static
void ugh_client_wcb_timeout(EV_P_ ev_timer *w, int tev)
{
	ugh_client_t *c = aux_memberof(ugh_client_t, wev_timeout, w);
	ugh_client_del(c);
}

static
void ugh_client_wcb_send(EV_P_ ev_io *w, int tev)
{
	int rc;
	ugh_client_t *c = aux_memberof(ugh_client_t, wev_send, w);

	if (0 == c->buf_send.size)
	{
		ugh_client_del(c);
		return;
	}

	if (NULL == c->iov)
	{
		c->iov_num = ugh_module_handles_size + 1;

		c->iov = aux_pool_malloc(c->pool, c->iov_num * sizeof(c->iov[0]));

		if (NULL == c->iov)
		{
			ugh_client_del(c);
			return;
		}

		c->iov[0].iov_base = c->buf_send.data;
		c->iov[0].iov_len  = c->buf_send.size;

		size_t i;

		for (i = 0; i < c->iov_num - 1; ++i)
		{
			c->iov[i+1].iov_base = c->bufs[i].data;
			c->iov[i+1].iov_len  = c->bufs[i].size;
		}
	}

	rc = writev(w->fd, c->iov, c->iov_num);
	log_debug("client send: %d: %.*s", rc, (int) c->iov[0].iov_len, (char *) c->iov[0].iov_base);

	if (0 > rc)
	{
		log_warn("err writev(%d,%d) (%d: %s)", w->fd, rc, errno, aux_strerror(errno));
		ugh_client_del(c);
		return;
	}

	/* log_debug("end writev(%d,%d)", w->fd, rc); */

	if (0 == rc)
	{
		return;
	}

	size_t i;

	for (i = 0; i < c->iov_num; ++i)
	{
		if (rc < c->iov[i].iov_len)
		{
			c->iov[i].iov_base += rc;
			c->iov[i].iov_len  -= rc;

			c->iov += i;
			c->iov_num -= i;

			/* TODO send timeout */

			return;
		}

		rc -= c->iov[i].iov_len;
	}

	ugh_client_del(c);
}

static
void ugh_client_ccb_handle(void *arg)
{
	ugh_client_t *c = arg;

	int status = ugh_module_handle_all(c);

	if (UGH_AGAIN == status)
	{
		return;
	}

	ugh_client_send(c, status);
}

static
void ugh_client_wcb_recv(EV_P_ ev_io *w, int tev)
{
	int nb;
	ugh_client_t *c = aux_memberof(ugh_client_t, wev_recv, w);

	nb = aux_unix_recv(w->fd, c->buf_recv.data, c->buf_recv.size);
	log_debug("client recv: %d: %.*s", nb, nb, c->buf_recv.data);

	if (0 == nb)
	{
		ugh_client_del(c);
		return;
	}

	if (0 > nb)
	{
		if (EAGAIN == errno)
		{
			ev_timer_again(loop, &c->wev_timeout);
			return;
		}

		ugh_client_del(c);
		return;
	}

	c->buf_recv.data += nb;
	c->buf_recv.size -= nb;

	ev_timer_again(loop, &c->wev_timeout);

	if (NULL == c->request_end)
	{
		int status = ugh_parser_client(c, c->buf_recv.data - nb, nb);

		if (UGH_AGAIN == status)
		{
			return;
		}

		if (UGH_HTTP_BAD_REQUEST <= status)
		{
			ugh_client_send(c, status);
			return;
		}

		if (UGH_HTTP_POST == c->method)
		{
			ugh_header_t *hdr_content_length = ugh_client_header_get_nt(c, "Content-Length");

			if (0 != hdr_content_length->value.size)
			{
				c->content_length = atoi(hdr_content_length->value.data);

				if (c->content_length > (c->buf_recv.size + (c->buf_recv.data - c->request_end)))
				{
					c->body.data = aux_pool_nalloc(c->pool, c->content_length);
					c->body.size = c->buf_recv.data - c->request_end;

					memcpy(c->body.data, c->request_end, c->body.size);

					c->buf_recv.data = c->body.data + c->body.size;
					c->buf_recv.size = c->content_length - c->body.size;
				}
				else
				{
					c->body.data = c->request_end;
					c->body.size = c->buf_recv.data - c->request_end;
				}

				if (c->body.size < c->content_length)
				{
					return;
				}
			}
		}
	}
	else if (UGH_HTTP_POST == c->method)
	{
		c->body.size += nb;

		if (c->body.size < c->content_length)
		{
			return;
		}
	}

	ev_io_stop(loop, &c->wev_recv);
	ev_timer_stop(loop, &c->wev_timeout);

#if 1 /* prepare post args */
	ugh_header_t *hdr_content_type = ugh_client_header_get_nt(c, "Content-Type");

	if (sizeof("application/x-www-form-urlencoded") - 1 == hdr_content_type->value.size &&
		0 == strncmp(hdr_content_type->value.data, "application/x-www-form-urlencoded", hdr_content_type->value.size))
	{
		ugh_parser_client_body(c, c->body.data, c->body.size);
	}
#endif

#if 1 /* UGH_CORO ENABLE */
	c->stack = aux_pool_malloc(c->pool, UGH_CORO_STACK);

	if (NULL == c->stack)
	{
		ugh_client_send(c, UGH_HTTP_INTERNAL_SERVER_ERROR);
		return;
	}

	coro_create(&c->ctx, ugh_client_ccb_handle, c, c->stack, UGH_CORO_STACK, &ctx_main);
	is_main_coro = 0;
	coro_transfer(&ctx_main, &c->ctx);
	is_main_coro = 1;
#endif

#if 0 /* UGH_CORO DISABLE */
	ugh_client_ccb_handle(c);
#endif
}

int ugh_client_send(ugh_client_t *c, int status)
{
#if 0
	if (0 == c->content_type.size)
	{
		c->content_type.size = sizeof("text/plain") - 1;
		c->content_type.data = "text/plain";
	}
#endif

	size_t i;

	for (i = 0; i < ugh_module_handles_size; ++i)
	{
		c->bufs_sumlen += c->bufs[i].size;
	}

	c->buf_send.data = (char *) aux_pool_nalloc(c->pool, UGH_HEADER_BUF);
	c->buf_send.size = snprintf(c->buf_send.data, UGH_HEADER_BUF,
		"HTTP/1.1 %s"              CRLF
		"Server: ugh/"UGH_VERSION  CRLF
		"Content-Length: %"PRIuMAX CRLF
		"Connection: close"        CRLF
		/* "Content-Type: %.*s"       CRLF */
		, ugh_status_header[status]
		, (uintmax_t) c->bufs_sumlen
		/* , (int) c->content_type.size, c->content_type.data */
	);

	void **vptr;
	Word_t idx = 0;

	for (vptr = JudyLFirst(c->headers_out_hash, &idx, PJE0); vptr; vptr = JudyLNext(c->headers_out_hash, &idx, PJE0))
	{
		ugh_header_t *h = *vptr;

		c->buf_send.size += snprintf(c->buf_send.data + c->buf_send.size, UGH_HEADER_BUF - c->buf_send.size,
			"%.*s: %.*s" CRLF, (int) h->key.size, h->key.data, (int) h->value.size, h->value.data);
	}

#if 0
	if (0 != c->location.size)
	{
		c->buf_send.size += snprintf(c->buf_send.data + c->buf_send.size, UGH_HEADER_BUF - c->buf_send.size,
			"Location: %.*s" CRLF, (int) c->location.size, c->location.data);
	}
#endif

	c->buf_send.size += snprintf(c->buf_send.data + c->buf_send.size, UGH_HEADER_BUF - c->buf_send.size, CRLF);

	log_notice("access %s:%u '%.*s%s%.*s' %.*s %"PRIuMAX, inet_ntoa(c->addr.sin_addr), ntohs(c->addr.sin_port), (int) c->uri.size, c->uri.data,
		c->args.size ? "?" : "", (int) c->args.size, c->args.data, 3, ugh_status_header[status],
		(uintmax_t) c->bufs_sumlen);

	ev_io_start(loop, &c->wev_send);

	return 0;
}

int ugh_client_add(ugh_server_t *s, int sd, struct sockaddr_in *addr)
{
	aux_pool_t *pool;
	ugh_client_t *c;

	pool = aux_pool_init(0);
	if (NULL == pool) return -1;

	c = (ugh_client_t *) aux_pool_calloc(pool, sizeof(*c));

	if (NULL == c)
	{
		aux_pool_free(pool);
		return -1;
	}

	c->s = s;

	c->addr.sin_family = addr->sin_family;
	c->addr.sin_addr.s_addr = addr->sin_addr.s_addr;
	c->addr.sin_port = addr->sin_port;

	c->pool = pool;

	c->buf_recv.size = UGH_HEADER_BUF;
	c->buf_recv.data = aux_pool_nalloc(pool, UGH_HEADER_BUF);

	if (NULL == c->buf_recv.data)
	{
		aux_pool_free(pool);
		return -1;
	}

	c->bufs = aux_pool_calloc(c->pool, ugh_module_handles_size * sizeof(*c->bufs));

	if (NULL == c->bufs)
	{
		aux_pool_free(pool);
		return -1;
	}

	ev_io_init(&c->wev_recv, ugh_client_wcb_recv, sd, EV_READ);
	ev_io_init(&c->wev_send, ugh_client_wcb_send, sd, EV_WRITE);
	ev_timer_init(&c->wev_timeout, ugh_client_wcb_timeout, 0, UGH_CONFIG_SOCKET_TIMEOUT);
	ev_timer_again(loop, &c->wev_timeout);
	ev_io_start(loop, &c->wev_recv);

	return 0;
}

int ugh_client_del(ugh_client_t *c)
{
	ev_io_stop(loop, &c->wev_recv);
	ev_io_stop(loop, &c->wev_send);
	ev_timer_stop(loop, &c->wev_timeout);

	close(c->wev_recv.fd);

	JudyLFreeArray(&c->args_hash, PJE0);
	JudyLFreeArray(&c->headers_hash, PJE0);
	JudyLFreeArray(&c->headers_out_hash, PJE0);
#if 1
	JudyLFreeArray(&c->vars_hash, PJE0);
#endif
	aux_pool_free(c->pool);

	return 0;
}

strp ugh_client_getarg_nt(ugh_client_t *c, const char *data)
{
	void **dest = JudyLGet(c->args_hash, aux_hash_key_nt(data), PJE0);
	if (PJERR == dest || NULL == dest) return &aux_empty_string;

	return *dest;
}

strp ugh_client_getarg(ugh_client_t *c, const char *data, size_t size)
{
	void **dest = JudyLGet(c->args_hash, aux_hash_key(data, size), PJE0);
	if (PJERR == dest || NULL == dest) return &aux_empty_string;

	return *dest;
}

strp ugh_client_setarg(ugh_client_t *c, const char *data, size_t size, char *value_data, size_t value_size)
{
	void **dest = JudyLIns(&c->args_hash, aux_hash_key(data, size), PJE0);
	if (PJERR == dest) return NULL;

	strp vptr = aux_pool_malloc(c->pool, sizeof(*vptr));
	if (NULL == vptr) return NULL;

	*dest = vptr;

	vptr->data = value_data;
	vptr->size = value_size;

	return vptr;
}

strp ugh_client_body_getarg_nt(ugh_client_t *c, const char *data)
{
	void **dest = JudyLGet(c->body_hash, aux_hash_key_nt(data), PJE0);
	if (PJERR == dest || NULL == dest) return &aux_empty_string;

	return *dest;
}

strp ugh_client_body_getarg(ugh_client_t *c, const char *data, size_t size)
{
	void **dest = JudyLGet(c->body_hash, aux_hash_key(data, size), PJE0);
	if (PJERR == dest || NULL == dest) return &aux_empty_string;

	return *dest;
}

strp ugh_client_body_setarg(ugh_client_t *c, const char *data, size_t size, char *value_data, size_t value_size)
{
	void **dest = JudyLIns(&c->body_hash, aux_hash_key(data, size), PJE0);
	if (PJERR == dest) return NULL;

	strp vptr = aux_pool_malloc(c->pool, sizeof(*vptr));
	if (NULL == vptr) return NULL;

	*dest = vptr;

	vptr->data = value_data;
	vptr->size = value_size;

	return vptr;
}

static ugh_header_t ugh_empty_header = {
	{ 0, "" },
	{ 0, "" }
};

ugh_header_t *ugh_client_header_get_nt(ugh_client_t *c, const char *data)
{
	void **dest = JudyLGet(c->headers_hash, aux_hash_key_lc_header_nt(data), PJE0);
	if (PJERR == dest || NULL == dest) return &ugh_empty_header;

	return *dest;
}

ugh_header_t *ugh_client_header_get(ugh_client_t *c, const char *data, size_t size)
{
	void **dest = JudyLGet(c->headers_hash, aux_hash_key_lc_header(data, size), PJE0);
	if (PJERR == dest || NULL == dest) return &ugh_empty_header;

	return *dest;
}

ugh_header_t *ugh_client_header_set(ugh_client_t *c, const char *data, size_t size, char *value_data, size_t value_size)
{
	void **dest = JudyLIns(&c->headers_hash, aux_hash_key_lc_header(data, size), PJE0);
	if (PJERR == dest) return NULL;

	ugh_header_t *vptr = aux_pool_malloc(c->pool, sizeof(*vptr));
	if (NULL == vptr) return NULL;

	*dest = vptr;

	vptr->key.data = (char *) data;
	vptr->key.size = size;
	vptr->value.data = value_data;
	vptr->value.size = value_size;

	return vptr;
}

ugh_header_t *ugh_client_header_out_set(ugh_client_t *c, const char *data, size_t size, char *value_data, size_t value_size)
{
	void **dest = JudyLIns(&c->headers_out_hash, aux_hash_key_lc_header(data, size), PJE0);
	if (PJERR == dest) return NULL;

	ugh_header_t *vptr = aux_pool_malloc(c->pool, sizeof(*vptr));
	if (NULL == vptr) return NULL;

	*dest = vptr;

	vptr->key.data = (char *) data;
	vptr->key.size = size;
	vptr->value.data = value_data;
	vptr->value.size = value_size;

	return vptr;
}

strp ugh_client_cookie_get(ugh_client_t *c, const char *name, size_t size)
{
	ugh_header_t *h = ugh_client_header_get_nt(c, "cookie");
	if (NULL == h) return &aux_empty_string;

	char *p = h->value.data;
	char *e = p + h->value.size;

	while (p < e)
	{
		if (0 != strncasecmp(p, name, size))
		{
			goto skip;
		}

		for (p += size; p < e && *p == ' '; ++p)
		{
			/* void */
		}

		if (p == e || *p != '=')
		{
			goto skip;
		}

		for (++p; p < e && *p == ' '; ++p)
		{
			/* void */
		}

		strp res = aux_pool_malloc(c->pool, sizeof(*res));
		if (NULL == res) return &aux_empty_string;

		res->data = p;

		for (; p < e && *p != ';'; ++p)
		{
			/* void */
		}

		res->size = p - res->data;

		return res;

skip:
		for (; p < e && *p != ';'; ++p)
		{
			/* void */
		}

		for (++p; p < e && *p == ' '; ++p)
		{
			/* void */
		}
	}

	return &aux_empty_string;
}

strp ugh_client_getvar(ugh_client_t *c, const char *data, size_t size)
{
	void **dest = JudyLGet(c->vars_hash, aux_hash_key(data, size), PJE0);
	if (PJERR == dest || NULL == dest) return &aux_empty_string;

	return *dest;
}

strp ugh_client_getvar_nt(ugh_client_t *c, const char *data)
{
	void **dest = JudyLGet(c->vars_hash, aux_hash_key_nt(data), PJE0);
	if (PJERR == dest || NULL == dest) return &aux_empty_string;

	return *dest;
}

strp ugh_client_setvar(ugh_client_t *c, const char *data, size_t size, char *value_data, size_t value_size)
{
	void **dest = JudyLIns(&c->vars_hash, aux_hash_key(data, size), PJE0);
	if (PJERR == dest) return NULL;

	strp vptr = aux_pool_malloc(c->pool, sizeof(*vptr));
	if (NULL == vptr) return NULL;

	*dest = vptr;

	vptr->data = value_data;
	vptr->size = value_size;

	return vptr;
}

strp ugh_client_setvar_nt(ugh_client_t *c, const char *data, char *value_data, size_t value_size)
{
	void **dest = JudyLIns(&c->vars_hash, aux_hash_key_nt(data), PJE0);
	if (PJERR == dest) return NULL;

	strp vptr = aux_pool_malloc(c->pool, sizeof(*vptr));
	if (NULL == vptr) return NULL;

	*dest = vptr;

	vptr->data = value_data;
	vptr->size = value_size;

	return vptr;
}

strp ugh_client_setvar_cp(ugh_client_t *c, const char *data, char *value_data, size_t value_size)
{
	void **dest = JudyLIns(&c->vars_hash, aux_hash_key_nt(data), PJE0);
	if (PJERR == dest) return NULL;

	strp vptr = aux_pool_malloc(c->pool, sizeof(*vptr));
	if (NULL == vptr) return NULL;

	*dest = vptr;

	vptr->data = aux_pool_nalloc(c->pool, value_size);
	if (NULL == vptr->data) return NULL;

	vptr->size = aux_cpymsz(vptr->data, value_data, value_size);

	return vptr;
}

strp ugh_client_setvar_va(ugh_client_t *c, const char *data, const char *fmt, ...)
{
	void **dest = JudyLIns(&c->vars_hash, aux_hash_key_nt(data), PJE0);
	if (PJERR == dest) return NULL;

	strp vptr = aux_pool_malloc(c->pool, sizeof(*vptr));
	if (NULL == vptr) return NULL;

	*dest = vptr;

	va_list ap;

	va_start(ap, fmt);
	vptr->size = vsnprintf(NULL, 0, fmt, ap);

	vptr->data = aux_pool_nalloc(c->pool, vptr->size + 1);
	if (NULL == vptr->data) return NULL;

	va_start(ap, fmt);
	vptr->size = vsnprintf(vptr->data, vptr->size + 1, fmt, ap);

	return vptr;
}

