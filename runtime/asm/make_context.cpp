#include "asm_context.h"
#include <stddef.h>
#include <string.h>
# include <stdlib.h>
#define NUM_SAVED 6
void make_context (asm_context *ctx, run_func fn, void *arg, void *sptr, size_t ssize)
{
  if (!fn)
    return;
  ctx->sp = (void **)(ssize + (char *)sptr);
  *--ctx->sp = (void *)abort; 
  *--ctx->sp = (void *)arg;
  *--ctx->sp = (void *)fn;
  ctx->sp -= NUM_SAVED;
  memset (ctx->sp, 0, sizeof (*ctx->sp) * NUM_SAVED);
}


