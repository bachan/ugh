#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "../ugh/aux/resolver.h"

int main(int argc, char **argv)
{
	if (3 > argc)
	{
		fprintf(stderr, "Usage: %s dns_server host [...]\n", argv[0]);
		return -1;
	}

	char q [4096];
	char a [4096];

	int sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (0 > sd) return -1;

	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(argv[1]);
	addr.sin_port = htons(53);

	int rc = connect(sd, (struct sockaddr *) &addr, addrlen);
	if (0 > rc) return -1;

	int i;

	for (i = 2; i < argc; ++i)
	{
		int qlen = create_name_query(q, argv[i], strlen(argv[i]));
		if (0 > qlen) continue;

		rc = send(sd, q, qlen, 0);
		/* printf("sent %d bytes\n", rc); */

		rc = recv(sd, a, 4096, 0);
		/* printf("recv %d bytes\n", rc); */

		/* debug */
		/* write(1, a, rc); */

		in_addr_t addrs [8];
		int naddrs = 8;
		int j;

		strt name;
		char name_data [1024];
		name.data = name_data;

		naddrs = process_response(a, rc, addrs, naddrs, &name);
#if 1
		printf("%s (name=%.*s):\n", argv[i], (int) name.size, name.data);

		for (j = 0; j < naddrs; ++j)
		{
			struct in_addr res_addr = { addrs[j] };
			printf("%s\n", inet_ntoa(res_addr));
		}
#endif
	}

	close(sd);

	return 0;
}


