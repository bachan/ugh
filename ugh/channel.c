#include "ugh.h"

static
int ugh_channel_del_memory(ugh_channel_t *ch)
{
	log_info("channel_id=%.*s del memory", (int) ch->channel_id.size, ch->channel_id.data);

	ev_async_stop(loop, &ch->wev_message);

	if (ch->timeout > 0)
	{
		ev_timer_stop(loop, &ch->wev_timeout);
	}

	JudyLFreeArray(&ch->messages_hash, PJE0);
	JudyLFreeArray(&ch->clients_hash, PJE0);

	aux_pool_free(ch->pool);

	return 0;
}

static
int ugh_channel_process_message(ugh_channel_t *ch)
{
	Word_t idx = 0;
	int rc;

	for (rc = Judy1First(ch->clients_hash, &idx, PJE0); 0 != rc;
		 rc = Judy1Next (ch->clients_hash, &idx, PJE0))
	{
		Judy1Unset(&ch->clients_hash, idx, PJE0);
		ugh_client_t *c = (ugh_client_t *) idx;

		is_main_coro = 0;
		coro_transfer(&ctx_main, &c->ctx);
		is_main_coro = 1;
	}

	/* Judy1FreeArray(&new_clients_hash, PJE0); */

	if (ch->status == UGH_CHANNEL_DELETED)
	{
		ugh_channel_del_memory(ch);
	}

	return 0;
}

static
void ugh_channel_wcb_timeout(EV_P_ ev_timer *w, int tev)
{
	ugh_channel_t *ch = aux_memberof(ugh_channel_t, wev_timeout, w);
	log_warn("channel_id=%.*s received timeout", (int) ch->channel_id.size, ch->channel_id.data);

	JudyLDel(&ch->s->channels_hash, aux_hash_key(ch->channel_id.data, ch->channel_id.size), PJE0);

	/* we can't remove proxy channel waiting for subrequest */
	/* TODO maybe we should force finish of subrequest here, but it's not that easy */
	if (ch->type == UGH_CHANNEL_PROXY && NULL != ch->subreqs_hash)
	{
		ev_timer_again(loop, w);
		return;
	}

	ch->status = UGH_CHANNEL_DELETED;
	ugh_channel_process_message(ch);
}

static
void ugh_channel_wcb_message(EV_P_ ev_async *w, int tev)
{
	ugh_channel_t *ch = aux_memberof(ugh_channel_t, wev_message, w);
	log_info("channel_id=%.*s received %s", (int) ch->channel_id.size, ch->channel_id.data, ch->status ? "delete" : "message");

	ugh_channel_process_message(ch);
}

ugh_channel_t *ugh_channel_add(ugh_server_t *s, strp channel_id, unsigned type, ev_tstamp timeout)
{
	/* check if channel already exist and return if it does */

	void **dest = JudyLIns(&s->channels_hash, aux_hash_key(channel_id->data, channel_id->size), PJE0);
	if (PJERR == dest) return NULL;

	if (NULL != *dest)
	{
		log_warn("channel_id=%.*s trying to add existing channel", (int) channel_id->size, channel_id->data);
		return *dest;
	}

	/* create new channel */

	aux_pool_t *pool;
	ugh_channel_t *ch;

	pool = aux_pool_init(0);
	if (NULL == pool) return NULL;

	ch = (ugh_channel_t *) aux_pool_calloc(pool, sizeof(*ch));

	if (NULL == ch)
	{
		aux_pool_free(pool);
		return NULL;
	}

	ch->s = s;
	ch->pool = pool;

	ev_async_init(&ch->wev_message, ugh_channel_wcb_message);
	ev_async_start(loop, &ch->wev_message);

	ch->timeout = timeout;

	if (ch->timeout > 0)
	{
		ev_timer_init(&ch->wev_timeout, ugh_channel_wcb_timeout, 0, ch->timeout);
		ev_timer_again(loop, &ch->wev_timeout);
	}

	ch->channel_id.size = channel_id->size;
	ch->channel_id.data = aux_pool_strdup(ch->pool, channel_id);
	if (NULL == ch->channel_id.data) return NULL;

	ch->type = type;

	*dest = ch;

	return ch;
}

ugh_channel_t *ugh_channel_get(ugh_server_t *s, strp channel_id)
{
	void **dest;

	dest = JudyLGet(s->channels_hash, aux_hash_key(channel_id->data, channel_id->size), PJE0);
	if (PJERR == dest || NULL == dest) return NULL;

	return *dest;
}

int ugh_channel_del(ugh_server_t *s, strp channel_id)
{
	Word_t idx = aux_hash_key(channel_id->data, channel_id->size);

	void **dest;

	dest = JudyLGet(s->channels_hash, idx, PJE0);
	if (PJERR == dest || NULL == dest) return -1;

	ugh_channel_t *ch = *dest;

	/* for now, we disallow removing proxy channels with DELETE requests */
	if (ch->type == UGH_CHANNEL_PROXY)
	{
		return -1;
	}

	ch->status = UGH_CHANNEL_DELETED;
	ev_async_send(loop, &ch->wev_message);

	/* 
	 * we remove channel from server list here, but free channel memory only
	 * after all clients will be served with 410 Gone respone
	 */

	JudyLDel(&s->channels_hash, idx, PJE0);

	return 0;
}

int ugh_channel_add_subreq(ugh_channel_t *ch, ugh_subreq_t *s)
{
	Judy1Set(&ch->subreqs_hash, (uintptr_t) s, PJE0);
	return 0;
}

int ugh_channel_add_message(ugh_channel_t *ch, strp body, strp content_type, ugh_subreq_t *r)
{
	Word_t etag = -1;
	void **dest;

	if (NULL != JudyLLast(ch->messages_hash, &etag, PJE0))
	{
		++etag;
	}
	else
	{
		etag = 1; /* first possible etag */
	}

	dest = JudyLIns(&ch->messages_hash, etag, PJE0);
	if (PJERR == dest) return -1;

	/* creating message */

	ugh_channel_message_t *m = aux_pool_malloc(ch->pool, sizeof(*m));
	if (NULL == m) return -1;

	if (NULL != r)
	{
		m->tag = r->tag;
	}

	m->body.data = aux_pool_strdup(ch->pool, body);
	m->body.size = body->size;

	m->content_type.data = aux_pool_strdup(ch->pool, content_type);
	m->content_type.size = content_type->size;

	*dest = m;

	ev_async_send(loop, &ch->wev_message);

	if (ch->type == UGH_CHANNEL_PROXY && NULL != r)
	{
		Judy1Unset(&ch->subreqs_hash, (uintptr_t) r, PJE0);
	}

	return 0;
}

static
int ugh_channel_gen_message(ugh_channel_t *ch, ugh_channel_message_t **m, Word_t *etag)
{
	void **dest = JudyLNext(ch->messages_hash, etag, PJE0);
	if (NULL == dest) return UGH_AGAIN; /* no new messages for this client */

	*m = *dest;

	/* check if we should remove PROXY channel now */

	if (ch->type == UGH_CHANNEL_PROXY && NULL == ch->subreqs_hash)
	{
		/* XXX this behaviour is arguable, cause we can try to use the same
		 * ugh_channel_t from client, which got last message */

		Word_t next_etag = *etag;

		if (NULL == (dest = JudyLNext(ch->messages_hash, &next_etag, PJE0))) /* check if next entry doesn't exist */
		{
			ch->status = UGH_CHANNEL_DELETED;
			ev_async_send(loop, &ch->wev_message);

			JudyLDel(&ch->s->channels_hash, aux_hash_key(ch->channel_id.data, ch->channel_id.size), PJE0);
		}
	}

	return UGH_OK;
}

int ugh_channel_get_message(ugh_channel_t *ch, ugh_client_t *c, ugh_channel_message_t **m, unsigned type, Word_t *etag)
{
	if (ch->status == UGH_CHANNEL_DELETED) /* XXX do we need this check here? */
	{
		return UGH_ERROR;
	}

	/* check if the next entry exist */

	Word_t original_etag = *etag;

	if (UGH_OK == ugh_channel_gen_message(ch, m, etag))
	{
		return UGH_OK;
	}

	*etag = original_etag;

	/* next entry was not found, we're waiting for it */

	if (type == UGH_CHANNEL_INTERVAL_POLL)
	{
		return UGH_AGAIN;
	}

	Judy1Set(&ch->clients_hash, (uintptr_t) c, PJE0);
	ch->clients_size++;

	for (;;) /* wait until we can generate message for this client */
	{
		is_main_coro = 1;
		coro_transfer(&c->ctx, &ctx_main);
		is_main_coro = 0;

		if (ch->status == UGH_CHANNEL_DELETED)
		{
			return UGH_ERROR;
		}

		if (UGH_OK == ugh_channel_gen_message(ch, m, etag))
		{
			break;
		}
	}

	return UGH_OK;
}

