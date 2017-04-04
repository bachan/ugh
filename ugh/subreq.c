#include "ugh.h"

static
void ugh_subreq_wcb_timeout(EV_P_ ev_timer *w, int tev)
{
	ugh_subreq_t *r = aux_memberof(ugh_subreq_t, wev_timeout, w);
	ugh_subreq_del(r, UGH_UPSTREAM_FT_TIMEOUT, 0);
}

static
void ugh_subreq_wcb_timeout_connect(EV_P_ ev_timer *w, int tev)
{
	ugh_subreq_t *r = aux_memberof(ugh_subreq_t, wev_timeout_connect, w);
	ugh_subreq_del(r, UGH_UPSTREAM_FT_TIMEOUT_CONNECT, 0);
}

static
void ugh_subreq_wcb_connect(EV_P_ ev_io *w, int tev)
{
	ugh_subreq_t *r = aux_memberof(ugh_subreq_t, wev_connect, w);

	if (EV_READ & tev)
	{
		int optval = 0;
		socklen_t optlen = sizeof(optval);

		if (0 > getsockopt(w->fd, SOL_SOCKET, SO_ERROR, &optval, &optlen))
		{
			optval = errno;
		}

		ugh_subreq_del(r, UGH_UPSTREAM_FT_ERROR, optval);
		return;
	}

	ev_io_stop(loop, &r->wev_connect);
	ev_timer_stop(loop, &r->wev_timeout_connect);

	r->connection_time = ev_now(loop) - r->response_time;

	ev_io_start(loop, &r->wev_send);
}

static
void ugh_subreq_wcb_send(EV_P_ ev_io *w, int tev)
{
	int rc;

	ugh_subreq_t *r = aux_memberof(ugh_subreq_t, wev_send, w);

	/* errno = 0; */

	rc = aux_unix_send(w->fd, r->b_send.data + r->b_send.rpos, r->b_send.wpos - r->b_send.rpos);
	log_debug("subreq send: %d: %.*s", rc, (int) (r->b_send.wpos - r->b_send.rpos), r->b_send.data + r->b_send.rpos);

	if (0 > rc)
	{
		ugh_subreq_del(r, UGH_UPSTREAM_FT_ERROR, errno);
		return;
	}

	/* log_debug("send %d bytes", rc); */

	if (0 == rc)
	{
		return;
	}

	if (UGH_TIMEOUT_ONCE == r->timeout_type)
	{
		ev_timer_again(loop, &r->wev_timeout);
	}

	r->b_send.rpos += rc;

	if (r->b_send.rpos == r->b_send.wpos)
	{
		ev_io_stop(loop, &r->wev_send);
		ev_io_start(loop, &r->wev_recv);
		return;
	}
}

static
int ugh_subreq_copy_chunk(ugh_subreq_t *r, char *data, size_t size)
{
	if (r->chunk_body_size < r->body.size + size)
	{
		char *old_body = r->body.data;

		r->chunk_body_size *= 2;
		r->body.data = aux_pool_nalloc(r->c->pool, r->chunk_body_size);

		memcpy(r->body.data, old_body, r->body.size);
	}

	memcpy(r->body.data + r->body.size, data, size);
	r->body.size += size;

	return 0;
}

static
uint32_t ugh_subreq_get_ft_type(ugh_subreq_t *r)
{
	uint32_t ft_type = UGH_UPSTREAM_FT_OFF;

	switch (r->status)
	{
	case 400: ft_type = UGH_UPSTREAM_FT_HTTP_4XX; break;
	case 401: ft_type = UGH_UPSTREAM_FT_HTTP_4XX; break;
	case 402: ft_type = UGH_UPSTREAM_FT_HTTP_4XX; break;
	case 403: ft_type = UGH_UPSTREAM_FT_HTTP_4XX; break;
	case 404: ft_type = UGH_UPSTREAM_FT_HTTP_404; break;
	case 405: ft_type = UGH_UPSTREAM_FT_HTTP_4XX; break;
	case 406: ft_type = UGH_UPSTREAM_FT_HTTP_4XX; break;
	case 407: ft_type = UGH_UPSTREAM_FT_HTTP_4XX; break;
	case 408: ft_type = UGH_UPSTREAM_FT_HTTP_4XX; break;
	case 409: ft_type = UGH_UPSTREAM_FT_HTTP_4XX; break;
	case 410: ft_type = UGH_UPSTREAM_FT_HTTP_4XX; break;
	case 411: ft_type = UGH_UPSTREAM_FT_HTTP_4XX; break;
	case 412: ft_type = UGH_UPSTREAM_FT_HTTP_4XX; break;
	case 413: ft_type = UGH_UPSTREAM_FT_HTTP_4XX; break;
	case 414: ft_type = UGH_UPSTREAM_FT_HTTP_4XX; break;
	case 415: ft_type = UGH_UPSTREAM_FT_HTTP_4XX; break;
	case 416: ft_type = UGH_UPSTREAM_FT_HTTP_4XX; break;
	case 417: ft_type = UGH_UPSTREAM_FT_HTTP_4XX; break;
	case 500: ft_type = UGH_UPSTREAM_FT_HTTP_500; break;
	case 501: ft_type = UGH_UPSTREAM_FT_HTTP_5XX; break;
	case 502: ft_type = UGH_UPSTREAM_FT_HTTP_502; break;
	case 503: ft_type = UGH_UPSTREAM_FT_HTTP_503; break;
	case 504: ft_type = UGH_UPSTREAM_FT_HTTP_504; break;
	case 505: ft_type = UGH_UPSTREAM_FT_HTTP_5XX; break;
	case 506: ft_type = UGH_UPSTREAM_FT_HTTP_5XX; break;
	case 507: ft_type = UGH_UPSTREAM_FT_HTTP_5XX; break;
	}

	return ft_type;
}

static
void ugh_subreq_wcb_recv(EV_P_ ev_io *w, int tev)
{
	ugh_subreq_t *r = aux_memberof(ugh_subreq_t, wev_recv, w);

	/* errno = 0; */

	int nb = aux_unix_recv(w->fd, r->buf_recv.data, r->buf_recv.size);
	log_debug("subreq recv: %d: %.*s", nb, nb, r->buf_recv.data);

	if (0 == nb)
	{
		if (r->content_length != UGH_RESPONSE_CLOSE_AFTER_BODY)
		{
			/*
			 * NOTE: recv(2) will never fail with EPIPE, so I'm using it here
			 * to get meaningful error message in ugh_subreq_del'
			 */
			log_warn("upstream prematurely closed connection");
			ugh_subreq_del(r, UGH_UPSTREAM_FT_ERROR, EPIPE);
		}
		else
		{
			uint32_t ft_type = ugh_subreq_get_ft_type(r);
			ugh_subreq_del(r, ft_type, 0);
		}

		return;
	}

	if (0 > nb)
	{
		if (EAGAIN == errno)
		{
			if (UGH_TIMEOUT_ONCE == r->timeout_type)
			{
				ev_timer_again(loop, &r->wev_timeout);
			}

			return;
		}

		ugh_subreq_del(r, UGH_UPSTREAM_FT_ERROR, errno);
		return;
	}

	r->buf_recv.data += nb;
	r->buf_recv.size -= nb;

	if (UGH_TIMEOUT_ONCE == r->timeout_type)
	{
		ev_timer_again(loop, &r->wev_timeout);
	}

	if (NULL == r->body.data)
	{
		int status = ugh_parser_subreq(r, r->buf_recv.data - nb, nb);

		if (UGH_AGAIN == status)
		{
			return;
		}

		if (UGH_ERROR == status)
		{
			ugh_subreq_del(r, UGH_UPSTREAM_FT_INVALID_HEADER, 0);
			return;
		}

		ugh_header_t *hdr_content_length = ugh_subreq_header_get_nt(r, "Content-Length");

		// 1xx, 204 and 304 responses MUST NOT contain message body
		if ((r->status >= 100 && r->status < 200) || r->status == 204 || r->status == 304)
		{
			r->content_length = 0;
			r->body.data = r->request_end;
			r->body.size = 0;
		}
		else if (0 != hdr_content_length->value.size)
		{
			r->content_length = atoi(hdr_content_length->value.data);

			if (r->content_length > r->buf_recv.size + (r->buf_recv.data - r->request_end))
			{
				r->body.data = aux_pool_nalloc(r->c->pool, r->content_length);
				r->body.size = r->buf_recv.data - r->request_end;

				memcpy(r->body.data, r->request_end, r->body.size);

				r->buf_recv.data = r->body.data + r->body.size;
				r->buf_recv.size = r->content_length - r->body.size;
			}
			else
			{
				r->body.data = r->request_end;
				r->body.size = r->buf_recv.data - r->request_end;
			}
		}
		else
		{
			ugh_header_t *hdr_transfer_encoding = ugh_subreq_header_get_nt(r, "Transfer-Encoding");

			if (7 == hdr_transfer_encoding->value.size && 0 == strncmp(hdr_transfer_encoding->value.data, "chunked", 7))
			{
				r->content_length = UGH_RESPONSE_CHUNKED;

				r->chunk_body_size = UGH_CHUNKS_BUF;
				r->body.data = aux_pool_nalloc(r->c->pool, r->chunk_body_size);
				r->body.size = 0;

				char *next_chunk = r->request_end;

				for (;;)
				{
					status = ugh_parser_chunks(r, next_chunk, r->buf_recv.data - next_chunk);

					if (UGH_AGAIN == status)
					{
						r->buf_recv.size += r->buf_recv.data - r->request_end;
						r->buf_recv.data = r->request_end;
						r->chunk_start = 0;
						return;
					}

					if (UGH_ERROR == status)
					{
						ugh_subreq_del(r, UGH_UPSTREAM_FT_INVALID_HEADER, 0);
						return;
					}

					if (0 == r->chunk_size)
					{
						ugh_subreq_del(r, UGH_UPSTREAM_FT_OFF, 0);
						return;
					}

					size_t recv_len = r->buf_recv.data - r->chunk_start;

					if (r->chunk_size > recv_len)
					{
						ugh_subreq_copy_chunk(r, r->chunk_start, recv_len);

						r->chunk_size -= recv_len;

						r->buf_recv.size += r->buf_recv.data - r->request_end;
						r->buf_recv.data = r->request_end;

						r->chunk_start = r->buf_recv.data;

						break;
					}
					else
					{
						ugh_subreq_copy_chunk(r, r->chunk_start, r->chunk_size);

						next_chunk = r->chunk_start + r->chunk_size;

						r->chunk_size = 0;
						r->chunk_start = 0;
					}
				}
			}
			else /* http/1.0 close after body response */
			{
				r->content_length = UGH_RESPONSE_CLOSE_AFTER_BODY;

				r->body.data = r->request_end;
				r->body.size = r->buf_recv.data - r->request_end;

				if (r->buf_recv.size == 0)
				{
					char *old_body = r->body.data;

					r->body.data = aux_pool_nalloc(r->c->pool, r->body.size * 2);

					memcpy(r->body.data, old_body, r->body.size);

					r->buf_recv.data = r->body.data + r->body.size;
					r->buf_recv.size = r->body.size;
				}
			}
		}
	}
	else if (r->content_length == UGH_RESPONSE_CHUNKED)
	{
		for (;;)
		{
			char *next_chunk = r->buf_recv.data - nb;

			if (r->chunk_start)
			{
				size_t recv_len = r->buf_recv.data - r->chunk_start;

				if (r->chunk_size > recv_len)
				{
					ugh_subreq_copy_chunk(r, r->chunk_start, recv_len);

					r->chunk_size -= recv_len;

					r->buf_recv.data -= nb;
					r->buf_recv.size += nb;

					r->chunk_start = r->buf_recv.data;

					break;
				}
				else
				{
					ugh_subreq_copy_chunk(r, r->chunk_start, r->chunk_size);

					next_chunk = r->chunk_start + r->chunk_size;

					r->chunk_size = 0;
				}
			}

			int status = ugh_parser_chunks(r, next_chunk, r->buf_recv.data - next_chunk);

			if (UGH_AGAIN == status)
			{
				r->buf_recv.data -= nb;
				r->buf_recv.size += nb;
				r->chunk_start = 0;
				return;
			}

			if (UGH_ERROR == status)
			{
				ugh_subreq_del(r, UGH_UPSTREAM_FT_INVALID_HEADER, 0);
				return;
			}

			if (0 == r->chunk_size)
			{
				ugh_subreq_del(r, UGH_UPSTREAM_FT_OFF, 0);
			}
		}
	}
	else if (r->content_length == UGH_RESPONSE_CLOSE_AFTER_BODY)
	{
		r->body.size += nb;

		if (r->buf_recv.size == 0)
		{
			char *old_body = r->body.data;

			r->body.data = aux_pool_nalloc(r->c->pool, r->body.size * 2);

			memcpy(r->body.data, old_body, r->body.size);

			r->buf_recv.data = r->body.data + r->body.size;
			r->buf_recv.size = r->body.size;
		}
	}
	else
	{
		r->body.size += nb;
	}

	if (r->body.size == r->content_length)
	{
		uint32_t ft_type = ugh_subreq_get_ft_type(r);
		ugh_subreq_del(r, ft_type, 0);
	}
}

static
int ugh_subreq_connect(void *data, in_addr_t addr)
{
	ugh_subreq_t *r = (ugh_subreq_t *) data;

	/* XXX this is needed temporarily, so if we del this subrequest due to
	 * resov error, we will not close any valid file descriptor inside
	 * ugh_subreq_del routine
	 */
	r->wev_recv.fd = -1;

	/* start calculating response_time from the first try */
	if (r->response_time == 0)
	{
		r->response_time = ev_now(loop);
	}

	/* reset connection_time */
	r->connection_time = 0;

	if (INADDR_NONE == addr)
	{
		ugh_subreq_del(r, UGH_UPSTREAM_FT_ERROR, ENXIO);
		return -1;
	}

	r->addr.sin_addr.s_addr = addr;

	log_debug("ugh_subreq_connect(%s:%u)", inet_ntoa(r->addr.sin_addr), ntohs(r->addr.sin_port));

	int sd, rc;

	if (0 > (sd = socket(AF_INET, SOCK_STREAM, 0)))
	{
		ugh_subreq_del(r, UGH_UPSTREAM_FT_ERROR, errno);
		return -1;
	}

	if (0 > (rc = aux_set_nonblk(sd, 1)))
	{
		close(sd);
		ugh_subreq_del(r, UGH_UPSTREAM_FT_ERROR, errno);
		return -1;
	}

	rc = connect(sd, (struct sockaddr *) &r->addr, sizeof(r->addr));

	if (0 > rc && EINPROGRESS != errno)
	{
		close(sd);
		ugh_subreq_del(r, UGH_UPSTREAM_FT_ERROR, errno);
		return -1;
	}

	/* errno = 0; */

	ev_io_init(&r->wev_recv, ugh_subreq_wcb_recv, sd, EV_READ);
	ev_io_init(&r->wev_send, ugh_subreq_wcb_send, sd, EV_WRITE);
	ev_io_init(&r->wev_connect, ugh_subreq_wcb_connect, sd, EV_READ | EV_WRITE);

	if (UGH_TIMEOUT_FULL == r->timeout_type)
	{
		ev_tstamp new_timeout = r->timeout - (ev_now(loop) - r->response_time);

		if (new_timeout != r->timeout)
		{
			/* XXX this is just temporarily on info level */
			log_info("updating timeout from %f to %f (%.*s:%.*s%.*s%s%.*s, addr=%s:%u)"
				, r->timeout
				, new_timeout
				, (int) r->u.host.size, r->u.host.data
				, (int) r->u.port.size, r->u.port.data
				, (int) r->u.uri.size, r->u.uri.data
				, r->u.args.size ? "?" : ""
				, (int) r->u.args.size, r->u.args.data
				, inet_ntoa(r->addr.sin_addr)
				, ntohs(r->addr.sin_port)
			);
		}

		if (new_timeout < 0)
		{
			ugh_subreq_del(r, UGH_UPSTREAM_FT_TIMEOUT, 0);
			return -1;
		}

		ev_timer_init(&r->wev_timeout, ugh_subreq_wcb_timeout, 0, new_timeout);
	}
	else
	{
		ev_timer_init(&r->wev_timeout, ugh_subreq_wcb_timeout, 0, r->timeout);
	}

	ev_timer_init(&r->wev_timeout_connect, ugh_subreq_wcb_timeout_connect, 0, r->timeout_connect);

	ev_timer_again(loop, &r->wev_timeout);
	ev_timer_again(loop, &r->wev_timeout_connect);

	ev_io_start(loop, &r->wev_connect);

	return 0;
}

ugh_subreq_t *ugh_subreq_add(ugh_client_t *c, char *url, size_t size, int flags)
{
	ugh_subreq_t *r;

	aux_pool_link(c->pool); /* use c->pool for subreq allocations */

	r = (ugh_subreq_t *) aux_pool_calloc(c->pool, sizeof(*r));

	if (NULL == r)
	{
		aux_pool_free(c->pool);
		return NULL;
	}

	ugh_client_add_subreq(c, r);

	r->c = c;

	r->flags = flags;
	r->handle = NULL;

	r->timeout = UGH_CONFIG_SUBREQ_TIMEOUT; /* XXX this should be "no timeout" by default */
	r->timeout_connect = UGH_CONFIG_SUBREQ_TIMEOUT_CONNECT; /* XXX this should be "no timeout" by default */

	ugh_parser_url(&r->u, url, size);

	log_debug("ugh_subreq_add(%.*s -> host=%.*s, port=%.*s, uri=%.*s, args=%.*s)",
		(int) size, url,
		(int) r->u.host.size, r->u.host.data,
		(int) r->u.port.size, r->u.port.data,
		(int) r->u.uri.size, r->u.uri.data,
		(int) r->u.args.size, r->u.args.data
	);

	r->method = c->method;

	r->request_body.data = c->body.data;
	r->request_body.size = c->body.size;

	return r;
}

int ugh_subreq_set_method(ugh_subreq_t *r, unsigned char method)
{
	r->method = method;

	if (method != UGH_HTTP_POST)
	{
		r->request_body.data = NULL;
		r->request_body.size = 0;
	}

	return 0;
}

int ugh_subreq_set_body(ugh_subreq_t *r, char *body, size_t body_size)
{
	r->method = UGH_HTTP_POST;

	r->request_body.data = body;
	r->request_body.size = body_size;

	return 0;
}

int ugh_subreq_set_timeout(ugh_subreq_t *r, ev_tstamp timeout, int timeout_type)
{
	r->timeout = timeout;
	r->timeout_type = timeout_type;

	return 0;
}

int ugh_subreq_set_timeout_connect(ugh_subreq_t *r, ev_tstamp timeout)
{
	r->timeout_connect = timeout;

	return 0;
}

int ugh_subreq_set_channel(ugh_subreq_t *r, ugh_channel_t *ch, unsigned tag)
{
	r->ch = ch;
	r->tag = tag;

	ugh_channel_add_subreq(ch, r);

	return 0;
}

int ugh_subreq_run(ugh_subreq_t *r)
{
	/* buffers */

	if (0 > aux_buffer_init(&r->b_send, r->c->pool, UGH_SUBREQ_BUF))
	{
		aux_pool_free(r->c->pool);
		return -1;
	}

	r->buf_recv_data = aux_pool_nalloc(r->c->pool, UGH_SUBREQ_BUF);

	if (NULL == r->buf_recv_data)
	{
		aux_pool_free(r->c->pool);
		return -1;
	}

	r->buf_recv.data = r->buf_recv_data;
	r->buf_recv.size = UGH_SUBREQ_BUF;

	/* upstream */

	strp u_host = &r->u.host;

	r->upstream = ugh_upstream_get(r->c->s->cfg, r->u.host.data, r->u.host.size);

	if (NULL != r->upstream)
	{
		r->upstream_tries = 0;

		while (r->upstream_tries < r->upstream->values_size)
		{
			r->upstream_current = r->upstream->values_curr;

			r->upstream->values_curr += 1;
			r->upstream->values_curr %= r->upstream->values_size;

			ugh_upstream_server_t *us = &r->upstream->values[r->upstream_current];

			if (us->max_fails == 0 || us->fails < us->max_fails)
			{
				break;
			}

			if (ev_now(loop) - us->fail_start >= us->fail_timeout)
			{
				log_info("upstream %.*s:%u is marked as working again"
					, (int) us->host.size, us->host.data
					, us->port
				);

				us->fails = 0;
				us->fail_start = 0;
				break;
			}

			r->upstream_tries++;
		}

		if (r->upstream_tries == r->upstream->values_size)
		{
			if (0 < r->upstream->backup_values_size)
			{
				r->upstream->backup_values_curr += 1;
				r->upstream->backup_values_curr %= r->upstream->backup_values_size;

				u_host = &r->upstream->backup_values[r->upstream->backup_values_curr].host;
			}
			else
			{
				aux_pool_free(r->c->pool);
				return -1;
			}
		}
		else
		{
			u_host = &r->upstream->values[r->upstream_current].host;
		}
	}

	/* generate request */

	ugh_subreq_gen(r, u_host);

	/* resolve host */

	r->resolver_ctx = aux_pool_malloc(r->c->pool, sizeof(ugh_resolver_ctx_t));

	if (NULL == r->resolver_ctx)
	{
		aux_pool_free(r->c->pool);
		return -1;
	}

	if ((r->flags & UGH_SUBREQ_WAIT))
	{
		r->c->wait++;
	}

	r->resolver_ctx->handle = ugh_subreq_connect;
	r->resolver_ctx->data = r;

	if (NULL != r->upstream)
	{
		r->addr.sin_family = AF_INET;
		r->addr.sin_port = htons(r->upstream->values[r->upstream_current].port);

		if (0 > ugh_resolver_addq(r->c->s->resolver, u_host->data, u_host->size, r->resolver_ctx))
		{
			/* ugh_resolver_addq shall call ctx->handle with INADDR_NONE argument in all error cases */
			return -1;
		}
	}
	else
	{
		r->addr.sin_family = AF_INET;
		r->addr.sin_port = htons(strtoul(r->u.port.data, NULL, 10));

		if (0 > ugh_resolver_addq(r->c->s->resolver, r->u.host.data, r->u.host.size, r->resolver_ctx))
		{
			/* ugh_resolver_addq shall call ctx->handle with INADDR_NONE argument in all error cases */
			return -1;
		}
	}

	return 0;
}

int ugh_subreq_gen(ugh_subreq_t *r, strp u_host)
{
	ugh_client_t *c = r->c;

	/* generate request line */

	if (0 == r->u.uri.size)
	{
		aux_buffer_printf(&r->b_send, c->pool, "%s %.*s%s%.*s %s" CRLF
			, ugh_method_string[r->method]
			, (int) c->uri.size, c->uri.data
			, c->args.size ? "?" : ""
			, (int) c->args.size, c->args.data
			, ugh_version_string[c->version]
		);
	}
	else if (0 == r->u.args.size)
	{
		aux_buffer_printf(&r->b_send, c->pool, "%s %.*s%s%.*s %s" CRLF
			, ugh_method_string[r->method]
			, (int) r->u.uri.size, r->u.uri.data
			, c->args.size ? "?" : ""
			, (int) c->args.size, c->args.data
			, ugh_version_string[c->version]
		);
	}
	else
	{
		aux_buffer_printf(&r->b_send, c->pool, "%s %.*s?%.*s %s" CRLF
			, ugh_method_string[r->method]
			, (int) r->u.uri.size, r->u.uri.data
			, (int) r->u.args.size, r->u.args.data
			, ugh_version_string[c->version]
		);
	}

	/* set headers overloaded by module */

	Word_t idx = 0;
	void **vptr;

	for (vptr = JudyLFirst(r->headers_out_hash, &idx, PJE0); NULL != vptr;
		 vptr = JudyLNext (r->headers_out_hash, &idx, PJE0))
	{
		ugh_header_t *h = *vptr;

		/* we explicitly remove headers which were overloaded by empty value */
		if (h->value.size == 0)
		{
			continue;
		}

		/* we don't allow overloading Content-Length header' */
		if (14 == h->key.size && aux_hash_key_lc_header("Content-Length", 14) == aux_hash_key_lc_header(h->key.data, h->key.size))
		{
			continue;
		}

		aux_buffer_printf(&r->b_send, c->pool, "%.*s: %.*s" CRLF
			, (int) h->key.size, h->key.data
			, (int) h->value.size, h->value.data
		);
	}

	/* copy original request headers, change host header with new value */

	idx = 0;

	for (vptr = JudyLFirst(c->headers_hash, &idx, PJE0); NULL != vptr;
		 vptr = JudyLNext (c->headers_hash, &idx, PJE0))
	{
		ugh_header_t *h = *vptr;

		/* check if header was overloaded */
		void **dest = JudyLGet(r->headers_out_hash, aux_hash_key_lc_header(h->key.data, h->key.size), PJE0);
		if (PJERR != dest && NULL != dest) continue;

		if (4 == h->key.size && aux_hash_key_lc_header("Host", 4) == aux_hash_key_lc_header(h->key.data, h->key.size))
		{
			aux_buffer_printf(&r->b_send, c->pool, "Host: %.*s" CRLF
				, (int) u_host->size, u_host->data
			);
		}
		else if (14 == h->key.size && aux_hash_key_lc_header("Content-Length", 14) == aux_hash_key_lc_header(h->key.data, h->key.size))
		{
			continue; /* remove Content-Length header if it was in original request */
		}
		else
		{
			aux_buffer_printf(&r->b_send, c->pool, "%.*s: %.*s" CRLF
				, (int) h->key.size, h->key.data
				, (int) h->value.size, h->value.data
			);
		}
	}

	if (NULL != r->request_body.data)
	{
		aux_buffer_printf(&r->b_send, c->pool, "Content-Length: %"PRIuMAX CRLF
			, (uintmax_t) r->request_body.size
		);

		/* TODO Content-Type */

		aux_buffer_strcpy(&r->b_send, c->pool, CRLF);

		aux_buffer_memcpy(&r->b_send, c->pool, r->request_body.data, r->request_body.size);
	}
	else
	{
		aux_buffer_strcpy(&r->b_send, c->pool, CRLF);
	}

	/* log_debug("ugh_subreq(%.*s)", (int) r->buf_send.size, r->buf_send.data); */

	return 0;
}

strp ugh_subreq_get_host(ugh_subreq_t *r)
{
	if (NULL == r->upstream)
	{
		return &r->u.host;
	}

	if (r->upstream_tries < r->upstream->values_size)
	{
		return &r->upstream->values[r->upstream_current].host;
	}

	return &r->upstream->backup_values[r->upstream->backup_values_curr].host;
}

in_port_t ugh_subreq_get_port(ugh_subreq_t *r)
{
	if (NULL == r->upstream)
	{
		return atoi(r->u.port.data);
	}

	if (r->upstream_tries < r->upstream->values_size)
	{
		return r->upstream->values[r->upstream_current].port;
	}

	return r->upstream->backup_values[r->upstream->backup_values_curr].port;
}

int ugh_subreq_del(ugh_subreq_t *r, uint32_t ft_type, int ft_errno)
{
	ev_io_stop(loop, &r->wev_recv);
	ev_io_stop(loop, &r->wev_send);
	ev_io_stop(loop, &r->wev_connect);
	ev_timer_stop(loop, &r->wev_timeout);
	ev_timer_stop(loop, &r->wev_timeout_connect);

	close(r->wev_recv.fd);

	switch (ft_type)
	{
	case UGH_UPSTREAM_FT_ERROR:
		log_warn("connection or read/write error (%d: %s) on upstream socket (%.*s:%.*s%.*s%s%.*s, addr=%s:%u) while %.*s%s%.*s"
			, ft_errno
			, aux_strerror(ft_errno)
			, (int) r->u.host.size, r->u.host.data
			, (int) r->u.port.size, r->u.port.data
			, (int) r->u.uri.size, r->u.uri.data
			, r->u.args.size ? "?" : ""
			, (int) r->u.args.size, r->u.args.data
			, inet_ntoa(r->addr.sin_addr)
			, ntohs(r->addr.sin_port)
			, (int) r->c->uri.size, r->c->uri.data
			, r->c->args.size ? "?" : ""
			, (int) r->c->args.size, r->c->args.data
		);
		break;
	case UGH_UPSTREAM_FT_TIMEOUT:
		log_warn("upstream timeout (%.*s:%.*s%.*s%s%.*s, addr=%s:%u) while %.*s%s%.*s"
			, (int) r->u.host.size, r->u.host.data
			, (int) r->u.port.size, r->u.port.data
			, (int) r->u.uri.size, r->u.uri.data
			, r->u.args.size ? "?" : ""
			, (int) r->u.args.size, r->u.args.data
			, inet_ntoa(r->addr.sin_addr)
			, ntohs(r->addr.sin_port)
			, (int) r->c->uri.size, r->c->uri.data
			, r->c->args.size ? "?" : ""
			, (int) r->c->args.size, r->c->args.data
		);
		break;
	case UGH_UPSTREAM_FT_INVALID_HEADER:
		log_warn("invalid header in upstream response (%.*s:%.*s%.*s%s%.*s, addr=%s:%u) while %.*s%s%.*s"
			, (int) r->u.host.size, r->u.host.data
			, (int) r->u.port.size, r->u.port.data
			, (int) r->u.uri.size, r->u.uri.data
			, r->u.args.size ? "?" : ""
			, (int) r->u.args.size, r->u.args.data
			, inet_ntoa(r->addr.sin_addr)
			, ntohs(r->addr.sin_port)
			, (int) r->c->uri.size, r->c->uri.data
			, r->c->args.size ? "?" : ""
			, (int) r->c->args.size, r->c->args.data
		);
		break;
	case UGH_UPSTREAM_FT_HTTP_500:
	case UGH_UPSTREAM_FT_HTTP_502:
	case UGH_UPSTREAM_FT_HTTP_503:
	case UGH_UPSTREAM_FT_HTTP_504:
	case UGH_UPSTREAM_FT_HTTP_404:
	case UGH_UPSTREAM_FT_HTTP_5XX:
	case UGH_UPSTREAM_FT_HTTP_4XX:
		log_warn("error status %u in upstream response (%.*s:%.*s%.*s%s%.*s, addr=%s:%u) while %.*s%s%.*s"
			, r->status
			, (int) r->u.host.size, r->u.host.data
			, (int) r->u.port.size, r->u.port.data
			, (int) r->u.uri.size, r->u.uri.data
			, r->u.args.size ? "?" : ""
			, (int) r->u.args.size, r->u.args.data
			, inet_ntoa(r->addr.sin_addr)
			, ntohs(r->addr.sin_port)
			, (int) r->c->uri.size, r->c->uri.data
			, r->c->args.size ? "?" : ""
			, (int) r->c->args.size, r->c->args.data
		);
		break;
	case UGH_UPSTREAM_FT_TIMEOUT_CONNECT:
		log_warn("upstream connect timeout (%.*s:%.*s%.*s%s%.*s, addr=%s:%u) while %.*s%s%.*s"
			, (int) r->u.host.size, r->u.host.data
			, (int) r->u.port.size, r->u.port.data
			, (int) r->u.uri.size, r->u.uri.data
			, r->u.args.size ? "?" : ""
			, (int) r->u.args.size, r->u.args.data
			, inet_ntoa(r->addr.sin_addr)
			, ntohs(r->addr.sin_port)
			, (int) r->c->uri.size, r->c->uri.data
			, r->c->args.size ? "?" : ""
			, (int) r->c->args.size, r->c->args.data
		);
		break;
	}

	/* stop if full timeout is already ticked out */
	if (UGH_TIMEOUT_FULL == r->timeout_type && r->timeout < ev_now(loop) - r->response_time)
	{
		goto ok;
	}

	if (r->upstream && !(r->c->s->cfg->next_upstream & ft_type)) /* successful response */
	{
		ugh_upstream_server_t *us = &r->upstream->values[r->upstream_current];

		if (us->max_fails != 0 && (us->fails < us->max_fails || ev_now(loop) - us->fail_start >= us->fail_timeout))
		{
			if (us->fails >= us->max_fails) /* log this line only for upstreams, which were marked as non working */
			{
				log_info("upstream %.*s:%u is marked as working again"
					, (int) us->host.size, us->host.data
					, us->port
				);
			}

			us->fails = 0;
			us->fail_start = 0;
		}
	}

	if (r->upstream && r->upstream_tries < r->upstream->values_size
		&& (r->c->s->cfg->next_upstream & ft_type))
	{
		ugh_upstream_server_t *us = &r->upstream->values[r->upstream_current];

		if (us->max_fails != 0 && ++us->fails == us->max_fails)
		{
			us->fail_start = ev_now(loop);

			log_warn("upstream %.*s:%u is marked as non working for %.3f seconds after %u tries"
				, (int) us->host.size, us->host.data
				, us->port
				, us->fail_timeout
				, us->max_fails
			);
		}

		strp u_host;

		r->upstream_tries++;

		while (r->upstream_tries < r->upstream->values_size)
		{
			r->upstream_current += 1;
			r->upstream_current %= r->upstream->values_size;

			us = &r->upstream->values[r->upstream_current];

			if (us->max_fails == 0 || us->fails < us->max_fails)
			{
				break;
			}

			if (ev_now(loop) - us->fail_start >= us->fail_timeout)
			{
				log_info("upstream %.*s:%u is marked as working again"
					, (int) us->host.size, us->host.data
					, us->port
				);

				us->fails = 0;
				us->fail_start = 0;
				break;
			}

			r->upstream_tries++;
		}

		if (r->upstream_tries == r->upstream->values_size)
		{
			if (0 < r->upstream->backup_values_size)
			{
				r->upstream->backup_values_curr += 1;
				r->upstream->backup_values_curr %= r->upstream->backup_values_size;

				r->addr.sin_family = AF_INET;
				r->addr.sin_port = htons(r->upstream->backup_values[r->upstream->backup_values_curr].port);

				u_host = &r->upstream->backup_values[r->upstream->backup_values_curr].host;
			}
			else
			{
				goto ok;
			}
		}
		else
		{
			r->addr.sin_family = AF_INET;
			r->addr.sin_port = htons(r->upstream->values[r->upstream_current].port);

			u_host = &r->upstream->values[r->upstream_current].host;
		}

		r->state = 0; /* XXX maybe here we should reinit the whole subreq structure? */

		r->body.data = NULL;
		r->body.size = 0;

		r->b_send.rpos = 0;
		r->b_send.wpos = 0;

		r->buf_recv.data = r->buf_recv_data;
		r->buf_recv.size = UGH_SUBREQ_BUF;

		ugh_subreq_gen(r, u_host);

		JudyLFreeArray(&r->headers_hash, PJE0);

		return ugh_resolver_addq(r->c->s->resolver, u_host->data, u_host->size, r->resolver_ctx);
	}

ok:

	r->ft_type = ft_type;
	r->response_time = ev_now(loop) - r->response_time;

	if (r->connection_time == 0)
	{
		r->connection_time = r->response_time;
	}

	if (/* NULL == r->handle &&*/ (r->flags & UGH_SUBREQ_WAIT))
	{
		r->c->wait--;

		/* We check is_main_coro here, because we could possibly call
		 * ugh_subreq_del from module coroutine (e.g. when IP-address of
		 * subrequest was definitely mallformed) and in this case we don't need
		 * to call coro_transfer
		 */
		if (0 == r->c->wait && is_main_coro)
		{
			is_main_coro = 0;
			coro_transfer(&ctx_main, &r->c->ctx);
			is_main_coro = 1;
		}

		/* coro_transfer(&ctx_main, &r->c->ctx); */
	}

	if ((r->flags & UGH_SUBREQ_PUSH))
	{
		ugh_header_t *h_content_type = ugh_subreq_header_get_nt(r, "Content-Type");
		ugh_channel_add_message(r->ch, &r->body, &h_content_type->value, r);
	}

	/* JudyLFreeArray(&r->headers_hash, PJE0); [> this should be cleaned after module <] */
	JudyLFreeArray(&r->headers_out_hash, PJE0);

	aux_pool_free(r->c->pool);

	return 0;
}

int ugh_subreq_del_after_module(ugh_subreq_t *r)
{
	JudyLFreeArray(&r->headers_hash, PJE0);
	return 0;
}

void ugh_subreq_wait(ugh_client_t *c)
{
	if (0 < c->wait)
	{
		is_main_coro = 1;
		coro_transfer(&c->ctx, &ctx_main);
		is_main_coro = 0;
	}
}

#if 1
static ugh_header_t ugh_empty_header = {
	{ 0, "" },
	{ 0, "" }
};

ugh_header_t *ugh_subreq_header_get_nt(ugh_subreq_t *r, const char *data)
{
	void **dest = JudyLGet(r->headers_hash, aux_hash_key_lc_header_nt(data), PJE0);
	if (PJERR == dest || NULL == dest) return &ugh_empty_header;

	return *dest;
}

ugh_header_t *ugh_subreq_header_get(ugh_subreq_t *r, const char *data, size_t size)
{
	void **dest = JudyLGet(r->headers_hash, aux_hash_key_lc_header(data, size), PJE0);
	if (PJERR == dest || NULL == dest) return &ugh_empty_header;

	return *dest;
}

ugh_header_t *ugh_subreq_header_set(ugh_subreq_t *r, const char *data, size_t size, char *value_data, size_t value_size)
{
	void **dest = JudyLIns(&r->headers_hash, aux_hash_key_lc_header(data, size), PJE0);
	if (PJERR == dest) return NULL;

	ugh_header_t *vptr = aux_pool_malloc(r->c->pool, sizeof(*vptr));
	if (NULL == vptr) return NULL;

	*dest = vptr;

	vptr->key.data = (char *) data;
	vptr->key.size = size;
	vptr->value.data = value_data;
	vptr->value.size = value_size;

	return vptr;
}

ugh_header_t *ugh_subreq_header_out_get_nt(ugh_subreq_t *r, const char *data)
{
	void **dest = JudyLGet(r->headers_out_hash, aux_hash_key_lc_header_nt(data), PJE0);
	if (PJERR == dest || NULL == dest) return &ugh_empty_header;

	return *dest;
}

ugh_header_t *ugh_subreq_header_out_get(ugh_subreq_t *r, const char *data, size_t size)
{
	void **dest = JudyLGet(r->headers_out_hash, aux_hash_key_lc_header(data, size), PJE0);
	if (PJERR == dest || NULL == dest) return &ugh_empty_header;

	return *dest;
}

ugh_header_t *ugh_subreq_header_out_set(ugh_subreq_t *r, const char *data, size_t size, char *value_data, size_t value_size)
{
	void **dest = JudyLIns(&r->headers_out_hash, aux_hash_key_lc_header(data, size), PJE0);
	if (PJERR == dest) return NULL;

	ugh_header_t *vptr = aux_pool_malloc(r->c->pool, sizeof(*vptr));
	if (NULL == vptr) return NULL;

	*dest = vptr;

	vptr->key.data = (char *) data;
	vptr->key.size = size;
	vptr->value.data = value_data;
	vptr->value.size = value_size;

	return vptr;
}
#endif

