#include <stdio.h>
#include <stddef.h>
#include "hashes.h"
#include "resolver.h"

#define AUX_RESOLVE_A         1
#define AUX_RESOLVE_CNAME     5
#define AUX_RESOLVE_PTR       12
#define AUX_RESOLVE_MX        15
#define AUX_RESOLVE_TXT       16
#define AUX_RESOLVE_DNAME     39

#define AUX_RESOLVE_FORMERR   1
#define AUX_RESOLVE_SERVFAIL  2
#define AUX_RESOLVE_NXDOMAIN  3
#define AUX_RESOLVE_NOTIMP    4
#define AUX_RESOLVE_REFUSED   5

typedef struct {
	unsigned char ident_hi;
	unsigned char ident_lo;
	unsigned char flags_hi;
	unsigned char flags_lo;
	unsigned char nqs_hi;
	unsigned char nqs_lo;
	unsigned char nan_hi;
	unsigned char nan_lo;
	unsigned char nns_hi;
	unsigned char nns_lo;
	unsigned char nar_hi;
	unsigned char nar_lo;
} aux_resolver_query_t;

typedef struct {
	unsigned char type_hi;
	unsigned char type_lo;
	unsigned char class_hi;
	unsigned char class_lo;
} aux_resolver_qs_t;

typedef struct {
	unsigned char type_hi;
	unsigned char type_lo;
	unsigned char class_hi;
	unsigned char class_lo;
	unsigned char ttl [4];
	unsigned char len_hi;
	unsigned char len_lo;
} aux_resolver_an_t;

int create_name_query(char *p, char *name, size_t size)
{
	size_t nlen = size ? (1 + size + 1) : 1;
	size_t qlen = sizeof(aux_resolver_query_t) + nlen + sizeof(aux_resolver_qs_t);

	aux_resolver_query_t *qq = (aux_resolver_query_t *) p;

	uint16_t ident = aux_hash_key(name, size) & 0xffff;

	qq->ident_hi = (unsigned char) ((ident >> 8) & 0xff);
	qq->ident_lo = (unsigned char) (ident & 0xff);

	/* recursion query */
	qq->flags_hi = 1;
	qq->flags_lo = 0;

	/* one question */
	qq->nqs_hi = 0; qq->nqs_lo = 1;
	qq->nan_hi = 0; qq->nan_lo = 0;
	qq->nns_hi = 0; qq->nns_lo = 0;
	qq->nar_hi = 0; qq->nar_lo = 0;

	p += sizeof(aux_resolver_query_t) + nlen;

	aux_resolver_qs_t *qs = (aux_resolver_qs_t *) p;

	/* query type */
	qs->type_hi = 0;
	qs->type_lo = AUX_RESOLVE_A; /* TODO (unsigned char) ctx->type */

	/* IP query class */
	qs->class_hi = 0;
	qs->class_lo = 1;

	/* convert "www.example.com" to "\3www\7example\3com\0" */

	nlen = 0; p--; *p-- = '\0';

	char *s;

	for (s = name + size - 1; s >= name; --s)
	{
		if ('.' != *s)
		{
			*p = *s;
			++nlen;
		}
		else
		{
			if (0 == nlen)
			{
				return -1;
			}

			*p = (unsigned char) nlen;
			nlen = 0;
		}

		--p;
	}

	*p = (unsigned char) nlen;

	return qlen;
}

int process_response(char *data, size_t size, in_addr_t *addrs, strp name)
{
	aux_resolver_query_t *q = (aux_resolver_query_t *) data;

	/* uint16_t ident = (q->ident_hi << 8) + q->ident_lo; */
	uint16_t flags = (q->flags_hi << 8) + q->flags_lo;

	uint16_t nqs = (q->nqs_hi << 8) + q->nqs_lo;
	uint16_t nan = (q->nan_hi << 8) + q->nan_lo;

	/* printf("ident=%u, flags=%u, nqs=%u, nan=%u\n", ident, flags, nqs, nan); */

	if (0 == (flags & 0x8000)) /* invalid DNS response */
	{
		return -1;
	}

	uint code = flags & 0x7f;

	if (AUX_RESOLVE_FORMERR == code || AUX_RESOLVE_REFUSED < code) /* error in DNS response */
	{
		return -1;
	}

	if (1 != nqs) /* invalid number of questions */
	{
		return -1;
	}

	uint i = sizeof(aux_resolver_query_t);
	size_t len;

	name->size = 0;

	while (i < size)
	{
		if ('\0' == data[i])
		{
			break;
		}

		if (0 != name->size)
		{
			name->data[name->size++] = '.';
		}

		name->size += aux_cpymsz(name->data + name->size, &data[i+1], (unsigned char) data[i]);

		len = (unsigned char) data[i];
		i += 1 + len;
	}

	i++;

	if (i + sizeof(aux_resolver_qs_t) + nan * (2 + sizeof(aux_resolver_an_t)) > size) /* short DNS response */
	{
		return -1;
	}

	aux_resolver_qs_t *qs = (aux_resolver_qs_t *) &data[i];

	uint16_t qtype = (qs->type_hi << 8) + qs->type_lo;
	uint16_t qclass = (qs->class_hi << 8) + qs->class_lo;

	/* printf("resolver DNS response qtype=%u, qclass=%u\n", qtype, qclass); */

	if (1 != qclass) /* unknown query class */
	{
		return -1;
	}

	if (AUX_RESOLVE_A != qtype) /* unknown query type */
	{
		return -1;
	}

	/* process a */

	i += sizeof(aux_resolver_qs_t);

	uint a;
	in_addr_t addr = 0;
	char *cname = NULL;
	int naddrs = 0;

	for (a = 0; a < nan; ++a)
	{
		uint start = i;

		while (i < size)
		{
			if (data[i] & 0xc0)
			{
				i += 2;
				break;
			}

			if (data[i] == 0)
			{
				i++;

				if (i - start < 2) /* invalid name in DNS response */
				{
					return -1;
				}
			}

			i += 1 + (unsigned char) data[i];
		}

		if (i + sizeof(aux_resolver_an_t) >= size) /* short DNS response */
		{
			return -1;
		}

		aux_resolver_an_t *an = (aux_resolver_an_t *) &data[i];

		qtype = (an->type_hi << 8) + an->type_lo;
		len = (an->len_hi << 8) + an->len_lo;

		if (AUX_RESOLVE_A == qtype)
		{
			i += sizeof(aux_resolver_an_t);

			if (i + len > size) /* short DNS response */
			{
				return -1;
			}

			addr = htonl(
				(((unsigned char) data[i]) << 24) +
				(((unsigned char) data[i+1]) << 16) +
				(((unsigned char) data[i+2]) << 8) +
				((unsigned char) data[i+3])
			);
			addrs[naddrs++] = addr;

			i += len;
		}
		else if (AUX_RESOLVE_CNAME == qtype)
		{
			cname = &data[i] + sizeof(aux_resolver_an_t);
			i += sizeof(aux_resolver_an_t) + len;

			printf("cname %u (%p)\n", (unsigned int) len, cname); /* \6yandex\2ru\0 */
		}
		else if (AUX_RESOLVE_DNAME == qtype)
		{
			i += sizeof(aux_resolver_an_t) + len;
		}
		else
		{
			fprintf(stderr, "unexpected qtype=%u, len=%u\n", qtype, (unsigned int) len);
		}
	}

	return naddrs;
}

