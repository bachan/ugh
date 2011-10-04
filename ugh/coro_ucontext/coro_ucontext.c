#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include "coro_ucontext.h"

static coro_func coro_init_func;
static void *coro_init_arg;
static coro_context *new_coro, *create_coro;

static void
coro_init (void)
{
  volatile coro_func func = coro_init_func;
  volatile void *arg = coro_init_arg;

  coro_transfer (new_coro, create_coro);

  func ((void *)arg);

  /* the new coro returned. bad. just abort() for now */
  /* abort (); */
}

void
coro_create (coro_context *ctx, coro_func coro, void *arg, void *sptr, long ssize, coro_context *link)
{
  coro_context nctx;

  if (!coro)
    return;

  coro_init_func = coro;
  coro_init_arg  = arg;

  new_coro    = ctx;
  create_coro = &nctx;

#if 1
  getcontext (&(ctx->uc));

  ctx->uc.uc_link           = &(link->uc);
  ctx->uc.uc_stack.ss_sp    = sptr;
  ctx->uc.uc_stack.ss_size  = (size_t)ssize;
  ctx->uc.uc_stack.ss_flags = 0;

  makecontext (&(ctx->uc), (void (*)())coro_init, 0);
#endif

  coro_transfer (create_coro, new_coro);
}

#if 0
// ------------------------

#include <stdio.h>

coro_context ctx_main;
coro_context ctx_func;

void func(void *arg)
{
  printf("func: switching to main\n");
  coro_transfer(&ctx_func, &ctx_main);

  printf("func: returning\n");
}

int main(int argc, char **argv)
{
  char stack [4096];

  coro_create(&ctx_func, func, NULL, stack, 4096, &ctx_main);

  printf("main: switching to func\n");
  coro_transfer(&ctx_main, &ctx_func);

  printf("main: switching to func\n");
  coro_transfer(&ctx_main, &ctx_func);

  printf("main: returned\n");

  return 0;
}
#endif

