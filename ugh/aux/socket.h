#ifndef __AUX_SOCKET_H__
#define __AUX_SOCKET_H__

#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline
int aux_set_nonblk(int s, int value)
{
	return ioctl(s, FIONBIO, &value);
}

static inline
int aux_set_sckopt(int s, int level, int key, int value)
{
	return setsockopt(s, level, key, (void *) &value, sizeof(value));
}

static inline
int aux_unix_send(int fd, void *data, size_t size)
{
	return send(fd, data, size, MSG_NOSIGNAL);
}

static inline
int aux_unix_recv(int fd, void *data, size_t size)
{
	int rc;

	for (;;)
	{
		rc = recv(fd, data, size, 0);
		if (0 <= rc) break;

		if (EINTR != errno)
		{
			return -1;
		}
	}

	return rc;
}

/* NOTE: If we will rewrite aux_inet_addr to return already accumulated address
 * on first bad symbol, we'll need to fix the usage of this function in
 * ugh_resolver (where we are trying to aux_inet_addr(hostname) to check if
 * it's valid IP address or hostname, which we should resolve) */

static inline
in_addr_t aux_inet_addr(const char *data, size_t size)
{
	const char *p;
	in_addr_t addr;
	uintptr_t octet, n;

	addr = 0;
	octet = 0;
	n = 0;

	for (p = data; p < data + size; ++p)
	{
		char c = *p;

		if (c >= '0' && c <= '9')
		{
			octet = octet * 10 + (c - '0');
			continue;
		}

		if (c == '.' && octet < 256)
		{
			addr = (addr << 8) + octet;
			octet = 0;
			n++;
			continue;
		}

		return INADDR_NONE;
	}

	if (n != 3)
	{
		return INADDR_NONE;
	}

	if (octet < 256)
	{
		addr = (addr << 8) + octet;
		return htonl(addr);
	}

	return INADDR_NONE;
}

#ifdef __cplusplus
}
#endif

#endif /* __AUX_SOCKET_H__ */
