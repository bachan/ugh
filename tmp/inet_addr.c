#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char **argv)
{
	in_addr_t a = inet_addr(argv[1]);

	unsigned char oct1 = (a) & 0xff;
	unsigned char oct2 = (a >> 8) & 0xff;
	unsigned char oct3 = (a >> 16) & 0xff;
	unsigned char oct4 = (a >> 24) & 0xff;

	printf("%u.%u.%u.%u\n", oct1, oct2, oct3, oct4);

	return 0;
}
