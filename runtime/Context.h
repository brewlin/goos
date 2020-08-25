#ifndef RUNTIME_CONTEXT_H
#define RUNTIME_CONTEXT_H
#define  DEFAULT_STACK 8 * 1024
#include "asm_context.h"

class Context {
public:
    asm_context old_ctx;
    asm_context cur_ctx;
    run_func    _fn;
    void *      func_data;
    char *      sp;
    char *      bp;
public:
    Context(){};
    Context(run_func func,void *data);
    ~Context();
    static void context_run(void *arg);
    void        swap_in();
    void        swap_out();
    bool        is_end;
    void        reset();
};


#endif
