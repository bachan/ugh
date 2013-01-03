#include "ugh.h"

#define S_CLIENT_READY              0x00    /* INITIAL */
#define S_CLIENT_METHOD             0x01    /* REQUEST */
#define S_CLIENT_SPACES             0x02
#define S_CLIENT_URI                0x03
#define S_CLIENT_ARGS               0x04
#define S_CLIENT_ARGS_VALUE         0x05
#define S_CLIENT_PROT               0x06
#define S_CLIENT_PROT_H             0x07
#define S_CLIENT_PROT_HT            0x08
#define S_CLIENT_PROT_HTT           0x09
#define S_CLIENT_PROT_HTTP          0x0A
#define S_CLIENT_PROT_MAJOR         0x0B
#define S_CLIENT_PROT_MINOR         0x0C
#define S_CLIENT_PROT_FINAL         0x0D
#define S_CLIENT_LAST               0x0E
#define S_HEADER_FIRST              0x0F    /* HEADERS */
#define S_HEADER_READY              0x10
#define S_HEADER_PARAM              0x11
#define S_HEADER_VALUE_BEGIN        0x12
#define S_HEADER_VALUE              0x13
#define S_HEADER_VALUE_SPACE        0x14
#define S_HEADER_VALUE_FINAL        0x15
#define S_HEADER_BLOCK_FINAL        0x16

int ugh_parser_client(ugh_client_t *c, char *data, size_t size)
{
	ucht state = c->state;
	char *p = data;
	char *e = p + size;

	for (; p < e; ++p)
	{
		char ch = *p;

		switch (state)
		{
		case S_CLIENT_READY:
			switch (ch)
			{
			case CR : 
			case LF : break;
			case 'G': c->request_beg = p; c->method = UGH_HTTP_GET;  state = S_CLIENT_METHOD; break;
			case 'H': c->request_beg = p; c->method = UGH_HTTP_HEAD; state = S_CLIENT_METHOD; break;
			case 'P': c->request_beg = p; c->method = UGH_HTTP_POST; state = S_CLIENT_METHOD; break;
			default : return UGH_HTTP_BAD_REQUEST;
			}
			break;
		case S_CLIENT_METHOD:
			switch (ch)
			{
			case ' ': state = S_CLIENT_SPACES; break;
			}
			break;
		case S_CLIENT_SPACES:
			switch (ch)
			{
			case '/': c->uri.data = p; state = S_CLIENT_URI; break;
			case ' ': break;
			default : return UGH_HTTP_BAD_REQUEST;
			}
			break;
		case S_CLIENT_URI:
			switch (ch)
			{
			case ' ': c->uri.size = p - c->uri.data; state = S_CLIENT_PROT; break;
			case CR : c->uri.size = p - c->uri.data; c->version = UGH_HTTP_VERSION_0_9; state = S_CLIENT_LAST; break;
			case LF : c->uri.size = p - c->uri.data; c->version = UGH_HTTP_VERSION_0_9; state = S_HEADER_FIRST; break;
			case '?': c->uri.size = p - c->uri.data; c->args.data = p + 1; c->key_b = p + 1; state = S_CLIENT_ARGS; break;
			}
			break;
		case S_CLIENT_ARGS:
			switch (ch)
			{
			case ' ': c->args.size = p - c->args.data; state = S_CLIENT_PROT; break;
			case CR : c->args.size = p - c->args.data; c->version = UGH_HTTP_VERSION_0_9; state = S_CLIENT_LAST; break;
			case LF : c->args.size = p - c->args.data; c->version = UGH_HTTP_VERSION_0_9; state = S_HEADER_FIRST; break;
			case '&': c->key_b = p + 1; break;
			case '=': c->key_e = p; c->val_b = p + 1; state = S_CLIENT_ARGS_VALUE; break;
			}
			break;
		case S_CLIENT_ARGS_VALUE:
			switch (ch)
			{
			case ' ': ugh_client_setarg(c, c->key_b, c->key_e - c->key_b, c->val_b, p - c->val_b); c->args.size = p - c->args.data; state = S_CLIENT_PROT; break;
			case CR : ugh_client_setarg(c, c->key_b, c->key_e - c->key_b, c->val_b, p - c->val_b); c->args.size = p - c->args.data; c->version = UGH_HTTP_VERSION_0_9; state = S_CLIENT_LAST; break;
			case LF : ugh_client_setarg(c, c->key_b, c->key_e - c->key_b, c->val_b, p - c->val_b); c->args.size = p - c->args.data; c->version = UGH_HTTP_VERSION_0_9; state = S_HEADER_FIRST; break;
			case '&': ugh_client_setarg(c, c->key_b, c->key_e - c->key_b, c->val_b, p - c->val_b); c->key_b = p + 1; state = S_CLIENT_ARGS; break;
			}
			break;
		case S_CLIENT_PROT:
			switch (ch)
			{
			case ' ': break;
			case CR : c->version = UGH_HTTP_VERSION_0_9; state = S_CLIENT_LAST; break;
			case LF : c->version = UGH_HTTP_VERSION_0_9; state = S_HEADER_FIRST; break;
			case 'H': state = S_CLIENT_PROT_H; break;
			default : return UGH_HTTP_BAD_REQUEST;
			}
			break;
		case S_CLIENT_PROT_H:
			switch (ch)
			{
			case 'T': state = S_CLIENT_PROT_HT; break;
			default : return UGH_HTTP_BAD_REQUEST;
			}
			break;
		case S_CLIENT_PROT_HT:
			switch (ch)
			{
			case 'T': state = S_CLIENT_PROT_HTT; break;
			default : return UGH_HTTP_BAD_REQUEST;
			}
			break;
		case S_CLIENT_PROT_HTT:
			switch (ch)
			{
			case 'P': state = S_CLIENT_PROT_HTTP; break;
			default : return UGH_HTTP_BAD_REQUEST;
			}
			break;
		case S_CLIENT_PROT_HTTP:
			switch (ch)
			{
			case '/': state = S_CLIENT_PROT_MAJOR; break;
			default : return UGH_HTTP_BAD_REQUEST;
			}
			break;
		case S_CLIENT_PROT_MAJOR:
			switch (ch)
			{
			case '.': state = S_CLIENT_PROT_MINOR; break;
			case '0': c->version = UGH_HTTP_VERSION_0_9; break;
			case '1': c->version = UGH_HTTP_VERSION_1_0; break;
			default : return UGH_HTTP_BAD_REQUEST;
			}
			break;
		case S_CLIENT_PROT_MINOR:
			switch (ch)
			{
			case ' ': state = S_CLIENT_PROT_FINAL; break;
			case CR : state = S_CLIENT_LAST; break;
			case LF : state = S_HEADER_FIRST; break;
			case '0': c->version = UGH_HTTP_VERSION_1_0; break;
			case '1': c->version = UGH_HTTP_VERSION_1_1; break;
			case '9': c->version = UGH_HTTP_VERSION_0_9; break;
			default : return UGH_HTTP_BAD_REQUEST;
			}
			break;
		case S_CLIENT_PROT_FINAL:
			switch (ch)
			{
			case ' ': break;
			case CR : state = S_CLIENT_LAST; break;
			case LF : state = S_HEADER_FIRST; break;
			default : return UGH_HTTP_BAD_REQUEST;
			}
			break;
		case S_CLIENT_LAST:
			switch (ch)
			{
			case LF : state = S_HEADER_FIRST; break;
			default : return UGH_HTTP_BAD_REQUEST;
			}
			break;

		/* S_HEADER ####################################################### */

		case S_HEADER_FIRST:
			c->headers_beg = p;

			if (c->version < UGH_HTTP_VERSION_1_0 &&
				c->method != UGH_HTTP_GET)
			{
				return UGH_HTTP_NOT_ALLOWED;
			}

		case S_HEADER_READY:
			switch (ch)
			{
			case CR : state = S_HEADER_BLOCK_FINAL; break;
			case LF : goto final;
			default : c->key_b = p; state = S_HEADER_PARAM; break;
			}
			break;
		case S_HEADER_PARAM:
			switch (ch)
			{
			case ':': c->key_e = p; state = S_HEADER_VALUE_BEGIN; break;
			case CR : c->key_e = p; c->val_b = p; c->val_e = p; state = S_HEADER_VALUE_FINAL; break;
			case LF : c->key_e = p; c->val_b = p; c->val_e = p; ugh_client_header_set(c, c->key_b, c->key_e - c->key_b, c->val_b, c->val_e - c->val_b); state = S_HEADER_READY; break; /* insert header */
			}
			break;
		case S_HEADER_VALUE_BEGIN:
			switch (ch)
			{
			case ' ': break;
			case CR : c->val_b = p; c->val_e = p; state = S_HEADER_VALUE_FINAL; break;
			case LF : c->val_b = p; c->val_e = p; ugh_client_header_set(c, c->key_b, c->key_e - c->key_b, c->val_b, c->val_e - c->val_b); state = S_HEADER_READY; break; /* insert header */
			default : c->val_b = p; state = S_HEADER_VALUE; break;
			}
			break;
		case S_HEADER_VALUE:
			switch (ch)
			{
			case ' ': c->val_e = p; state = S_HEADER_VALUE_SPACE; break;
			case CR : c->val_e = p; state = S_HEADER_VALUE_FINAL; break;
			case LF : c->val_e = p; ugh_client_header_set(c, c->key_b, c->key_e - c->key_b, c->val_b, c->val_e - c->val_b); state = S_HEADER_READY; break; /* insert header */
			}
			break;
		case S_HEADER_VALUE_SPACE:
			switch (ch)
			{
			case ' ': break;
			case CR : state = S_HEADER_VALUE_FINAL; break;
			case LF : ugh_client_header_set(c, c->key_b, c->key_e - c->key_b, c->val_b, c->val_e - c->val_b); state = S_HEADER_READY; break; /* insert header */
			default : state = S_HEADER_VALUE; break;
			}
			break;
		case S_HEADER_VALUE_FINAL:
			switch (ch)
			{
			case LF : ugh_client_header_set(c, c->key_b, c->key_e - c->key_b, c->val_b, c->val_e - c->val_b); state = S_HEADER_READY; break; /* insert header */
			case CR : break;
			default : return UGH_HTTP_BAD_REQUEST;
			}
			break;
		case S_HEADER_BLOCK_FINAL:
			switch (ch)
			{
			case LF : goto final;
			default : return UGH_HTTP_BAD_REQUEST;
			}
			break;
		}
	}

	c->state = state;

	return UGH_AGAIN;

final:
	c->request_end = p + 1;
	c->state = S_CLIENT_READY;

	return UGH_OK;
}

#define S_CHUNKS_READY              0x00
#define S_CHUNKS_VALUE              0x01
#define S_CHUNKS_LAST               0x02

/* TODO chunk extensions (see nginx) */

int ugh_parser_chunks(ugh_subreq_t *r, char *data, size_t size)
{
	ucht state = r->state;
	char *p = data;
	char *e = data + size;

	for (; p < e; ++p)
	{
		char ch = *p;

		switch (state)
		{
		case S_CHUNKS_READY:
			switch (ch)
			{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9': r->chunk_size = ch - '0'; state = S_CHUNKS_VALUE; break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F': r->chunk_size = ch - 'A' + 10; state = S_CHUNKS_VALUE; break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f': r->chunk_size = ch - 'a' + 10; state = S_CHUNKS_VALUE; break;
			}
			break;
		case S_CHUNKS_VALUE:
			switch (ch)
			{
			case CR : state = S_CHUNKS_LAST; break;
			case LF : goto final;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9': r->chunk_size = r->chunk_size * 16 + ch - '0'; break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F': r->chunk_size = r->chunk_size * 16 + ch - 'A' + 10; break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f': r->chunk_size = r->chunk_size * 16 + ch - 'a' + 10; break;
			default : return UGH_ERROR;
			}
			break;
		case S_CHUNKS_LAST:
			switch (ch)
			{
			case LF : goto final;
			default : return UGH_ERROR;
			}
			break;
		}
	}

	r->state = state;

	return UGH_AGAIN;

final:
	r->chunk_start = p + 1;
	r->state = S_CHUNKS_READY;

	return UGH_OK;
}

#define S_SUBREQ_PROT               0x00
#define S_SUBREQ_PROT_H             0x01
#define S_SUBREQ_PROT_HT            0x02
#define S_SUBREQ_PROT_HTT           0x03
#define S_SUBREQ_PROT_HTTP          0x04
#define S_SUBREQ_PROT_MAJOR         0x05
#define S_SUBREQ_PROT_MINOR         0x06
#define S_SUBREQ_PROT_FINAL         0x07
#define S_SUBREQ_STATUS             0x08
#define S_SUBREQ_STATUS_TEXT        0x09
#define S_SUBREQ_LAST               0x0A

int ugh_parser_subreq(ugh_subreq_t *r, char *data, size_t size)
{
	ucht state = r->state;
	char *p = data;
	char *e = data + size;

	for (; p < e; ++p)
	{
		char ch = *p;

		switch (state)
		{
		case S_SUBREQ_PROT:
			switch (ch)
			{
			case 'H': r->request_beg = p; state = S_SUBREQ_PROT_H; break;
			}
			break;
		case S_SUBREQ_PROT_H:
			switch (ch)
			{
			case 'T': state = S_SUBREQ_PROT_HT; break;
			default : return UGH_ERROR;
			}
			break;
		case S_SUBREQ_PROT_HT:
			switch (ch)
			{
			case 'T': state = S_SUBREQ_PROT_HTT; break;
			default : return UGH_ERROR;
			}
			break;
		case S_SUBREQ_PROT_HTT:
			switch (ch)
			{
			case 'P': state = S_SUBREQ_PROT_HTTP; break;
			default : return UGH_ERROR;
			}
			break;
		case S_SUBREQ_PROT_HTTP:
			switch (ch)
			{
			case '/': state = S_SUBREQ_PROT_MAJOR; break;
			default : return UGH_ERROR;
			}
			break;
		case S_SUBREQ_PROT_MAJOR:
			switch (ch)
			{
			case '.': state = S_SUBREQ_PROT_MINOR; break;
			case '0': r->version = UGH_HTTP_VERSION_0_9; break;
			case '1': r->version = UGH_HTTP_VERSION_1_0; break;
			default : return UGH_ERROR;
			}
			break;
		case S_SUBREQ_PROT_MINOR:
			switch (ch)
			{
			case ' ': state = S_SUBREQ_PROT_FINAL; break;
			case '0': r->version = UGH_HTTP_VERSION_1_0; break;
			case '1': r->version = UGH_HTTP_VERSION_1_1; break;
			case '9': r->version = UGH_HTTP_VERSION_0_9; break;
			default : return UGH_ERROR;
			}
			break;
		case S_SUBREQ_PROT_FINAL:
			switch (ch)
			{
			case ' ': break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9': r->status = ch - '0'; state = S_SUBREQ_STATUS; break;
			default : return UGH_ERROR;
			}
			break;
		case S_SUBREQ_STATUS:
			switch (ch)
			{
			case ' ': state = S_SUBREQ_STATUS_TEXT; break;
			case CR : state = S_SUBREQ_LAST; break;
			case LF : state = S_HEADER_FIRST; break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9': r->status = r->status * 10 + ch - '0'; break;
			default : return UGH_ERROR;
			}
			break;
		case S_SUBREQ_STATUS_TEXT:
			switch (ch)
			{
			case CR : state = S_SUBREQ_LAST; break;
			case LF : state = S_HEADER_FIRST; break;
			}
			break;
		case S_SUBREQ_LAST:
			switch (ch)
			{
			case LF : state = S_HEADER_FIRST; break;
			default : return UGH_ERROR;
			}
			break;

		/* S_HEADER ####################################################### */

		case S_HEADER_FIRST:
			r->headers_beg = p;

		case S_HEADER_READY:
			switch (ch)
			{
			case CR : state = S_HEADER_BLOCK_FINAL; break;
			case LF : goto final;
			default : r->key_b = p; state = S_HEADER_PARAM; break;
			}
			break;
		case S_HEADER_PARAM:
			switch (ch)
			{
			case ':': r->key_e = p; state = S_HEADER_VALUE_BEGIN; break;
			case CR : r->key_e = p; r->val_b = p; r->val_e = p; state = S_HEADER_VALUE_FINAL; break;
			case LF : r->key_e = p; r->val_b = p; r->val_e = p; ugh_subreq_header_set(r, r->key_b, r->key_e - r->key_b, r->val_b, r->val_e - r->val_b); state = S_HEADER_READY; break; /* insert header */
			}
			break;
		case S_HEADER_VALUE_BEGIN:
			switch (ch)
			{
			case ' ': break;
			case CR : r->val_b = p; r->val_e = p; state = S_HEADER_VALUE_FINAL; break;
			case LF : r->val_b = p; r->val_e = p; ugh_subreq_header_set(r, r->key_b, r->key_e - r->key_b, r->val_b, r->val_e - r->val_b); state = S_HEADER_READY; break; /* insert header */
			default : r->val_b = p; state = S_HEADER_VALUE; break;
			}
			break;
		case S_HEADER_VALUE:
			switch (ch)
			{
			case ' ': r->val_e = p; state = S_HEADER_VALUE_SPACE; break;
			case CR : r->val_e = p; state = S_HEADER_VALUE_FINAL; break;
			case LF : r->val_e = p; ugh_subreq_header_set(r, r->key_b, r->key_e - r->key_b, r->val_b, r->val_e - r->val_b); state = S_HEADER_READY; break; /* insert header */
			}
			break;
		case S_HEADER_VALUE_SPACE:
			switch (ch)
			{
			case ' ': break;
			case CR : state = S_HEADER_VALUE_FINAL; break;
			case LF : ugh_subreq_header_set(r, r->key_b, r->key_e - r->key_b, r->val_b, r->val_e - r->val_b); state = S_HEADER_READY; break; /* insert header */
			default : state = S_HEADER_VALUE; break;
			}
			break;
		case S_HEADER_VALUE_FINAL:
			switch (ch)
			{
			case LF : ugh_subreq_header_set(r, r->key_b, r->key_e - r->key_b, r->val_b, r->val_e - r->val_b); state = S_HEADER_READY; break; /* insert header */
			case CR : break;
			default : return UGH_ERROR;
			}
			break;
		case S_HEADER_BLOCK_FINAL:
			switch (ch)
			{
			case LF : goto final;
			default : return UGH_ERROR;
			}
			break;
		}
	}

	r->state = state;

	return UGH_AGAIN;

final:
	r->request_end = p + 1;
	r->state = S_SUBREQ_PROT;

	return UGH_OK;
}

#define S_HOST 0
#define S_PORT 1
#define S_URI  2
#define S_ARGS 3

int ugh_parser_url(ugh_url_t *u, char *data, size_t size)
{
	char *b = data;
	char *p = data;
	char *e = data + size;

	ucht state = S_HOST;
	strp cur = &u->host;

	aux_clrptr(u);

	for (; p < e; ++p)
	{
		char ch = *p;

		switch (state)
		{
		case S_HOST:
			switch (ch)
			{
			case ':':
				cur->data = b;
				cur->size = p - b;
				state = S_PORT;
				cur = &u->port;
				b = p + 1;
				break;
			case '/':
				cur->data = b;
				cur->size = p - b;
				state = S_URI;
				cur = &u->uri;
				b = p;
				break;
			}
			break;
		case S_PORT:
			switch (ch)
			{
			case '/':
				cur->data = b;
				cur->size = p - b;
				state = S_URI;
				cur = &u->uri;
				b = p;
				break;
			}
			break;
		case S_URI:
			switch (ch)
			{
			case '?':
				cur->data = b;
				cur->size = p - b;
				state = S_ARGS;
				cur = &u->args;
				b = p + 1;
				break;
			}
			break;
		case S_ARGS:
			break;
		}
	}

	cur->data = b;
	cur->size = p - b;

	if (0 == u->port.size)
	{
		u->port.data = "80";
		u->port.size = sizeof("80") - 1;
	}

	return 0;
}

