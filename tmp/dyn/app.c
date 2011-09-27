#include <dlfcn.h>
#include <stdio.h>
#include "app.h"

int func(int arg)
{
	return arg;
}

int main(int argc, char **argv)
{
	if (2 > argc)
	{
		fprintf(stderr, "Usage: %s /path/to/library.so\n", argv[0]);
		return -1;
	}

	void *main_handle = dlopen(NULL, RTLD_NOW);
	printf("main_handle = %p\n", main_handle);

	char *path = argv[1];
	int flags = RTLD_NOW;

	void *handle = dlopen(path, flags);

	if (NULL == handle)
	{
		fprintf(stderr, "dlopen(%s, %d): %s\n", path, flags, dlerror());
		return -1;
	}

	return 0;
}

