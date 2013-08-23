#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "random.h"

int aux_random_init()
{
	int fd = open("/dev/urandom", O_RDONLY);
	if (0 > fd) return -1;

	uint32_t rnd_from_device;

	int rc = read(fd, &rnd_from_device, sizeof(rnd_from_device));
	if (0 > rc) return -1;

	close(fd);

	srandom(rnd_from_device);

	return 0;
}

