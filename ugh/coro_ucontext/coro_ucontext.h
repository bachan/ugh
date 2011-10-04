#ifndef __CORO_UCONTEXT_H__
#define __CORO_UCONTEXT_H__

typedef void (*coro_func)(void *);

typedef struct coro_context coro_context;

void coro_create (coro_context *ctx, /* an uninitialised coro_context */
                  coro_func coro,    /* the coroutine code to be executed */
                  void *arg,         /* a single pointer passed to the coro */
                  void *sptr,        /* start of stack area */
                  long ssize,        /* size of stack area */
                  coro_context *link);

#include <ucontext.h>

struct coro_context {
  ucontext_t uc;
};

#define coro_transfer(p,n) swapcontext (&((p)->uc), &((n)->uc))
#define coro_destroy(ctx) (void *)(ctx)

#endif /* __CORO_UCONTEXT_H__ */
