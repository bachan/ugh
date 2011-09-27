#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * int getaddrinfo(const char *node, const char *service,
 *         const struct addrinfo *hints,
 *         struct addrinfo **res);
 * 
 * void freeaddrinfo(struct addrinfo *res);
 * 
 * const char *gai_strerror(int errcode);
 * 
 * struct addrinfo {
 *     int		ai_flags;
 *     int		ai_family;
 *     int		ai_socktype;
 *     int		ai_protocol;
 *     size_t		ai_addrlen;
 *     struct sockaddr *ai_addr;
 *     char	       *ai_canonname;
 *     struct addrinfo *ai_next;
 * };
 */

int main(int argc, char **argv)
{
	int rc;
	struct addrinfo *res;

	rc = getaddrinfo(argv[1], argv[2], NULL, &res);

	if (0 != rc)
	{
		fprintf(stderr, "gai error: %d: %s\n", rc, gai_strerror(rc));
		freeaddrinfo(res);
		return -1;
	}

	for (; res->ai_next; res = res->ai_next)
	{
		printf("ai_flags     = %d\n", res->ai_flags);
		printf("ai_family    = %d\n", res->ai_family);
		printf("ai_socktype  = %d\n", res->ai_socktype);
		printf("ai_protocol  = %d\n", res->ai_protocol);
		printf("ai_addrlen   = %u\n", (unsigned) res->ai_addrlen);
		printf("ai_addr      = %s\n", inet_ntoa(((struct sockaddr_in *) res->ai_addr)->sin_addr));
		printf("ai_canonname = %s\n", res->ai_canonname);
		printf("\n");
	}

	freeaddrinfo(res);

	return 0;
}

