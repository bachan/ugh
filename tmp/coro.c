#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <coro/coro.h>

#define MAX_D 10

coro_context ctx_main;
coro_context ctx_func;

int d;
int done;

void func(void *arg)
{
	d = random() % MAX_D;

	printf("func: switching to main, d=%d, done=%d\n", d, done);
	coro_transfer(&ctx_func, &ctx_main);

	done = 1;

	printf("func: switching to main, d=%d, done=%d\n", d, done);
	coro_transfer(&ctx_func, &ctx_main);
}

int main(int argc, char **argv)
{
	char stack [4096];

	/* coro_create(&ctx_main, NULL, NULL, stack, 4096); */
	coro_create(&ctx_func, func, NULL, stack, 4096);

	srandom(1337);
	d = random() % MAX_D;

	done = 0;

	for (;;)
	{
		time_t t = time(NULL);
		printf("main: time=%d, d=%d, done=%d\n", (int) t, d, done);

		if (0 != done)
		{
			printf("main: done\n");
			break;
		}

		if (d == t % 10)
		{
			printf("main: switching to func, d=%d, done=%d\n", d, done);
			coro_transfer(&ctx_main, &ctx_func);
		}

		sleep(1);
	}

	return 0;
}

