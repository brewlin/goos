#ifndef ASM_CONTEXT_H
#define ASM_CONTEXT_H
#if __cplusplus
extern "C" {
#endif
#include <stddef.h>
typedef void (*run_func)(void *);
typedef  struct{
    void **sp;
}asm_context;

void make_context(asm_context *ctx,run_func fn,void *arg,void *sptr,size_t ssze);      
extern void __attribute__ ((__noinline__, __regparm__(2)))
jump_context (asm_context *prev, asm_context *next) asm("jump_context");

#if __cplusplus
}
#endif

#endif

