#include "Context.h"
#include "php_go.h"
#include "Coroutine.h"
/**
 * 创建c栈
 * @param func
 * @param data
 */
Context::Context(run_func func,void *data):_fn(func),func_data(data)
{
    bp =  new char[DEFAULT_STACK];
    make_context(&cur_ctx,&context_run, static_cast<void *>(this),bp,DEFAULT_STACK);
}
/**
 * 主要运行的函数
 * @param arg
 */
void Context::context_run(void *arg)
{
    Context *_this = static_cast<Context *>(arg);
    _this->_fn(_this->func_data);
    _this->is_end = true;
    GO_ZG(_g) = nullptr;
    _this->swap_out();
}
/**
 * 切入c栈
 */
void Context::swap_in()
{
//    Coroutine *co = static_cast<Coroutine*>(this->func_data);
//    GO_ZG(_g) =  co;
//    每次切入时出去时需要更新tick 和时间
//    GO_ZG(schedwhen) = chrono::steady_clock::now();
//    GO_ZG(schedtick) += 1;
//    co->gstatus = Grunnable;

    jump_context(&old_ctx,&cur_ctx);
}
/**
 * 切出c栈
 */
void Context::swap_out()
{
//    GO_ZG(_g) = nullptr;
//    每次切换出去时需要更新tick 和时间
//    GO_ZG(schedwhen) = chrono::steady_clock::now();
//    GO_ZG(schedtick) = 0;

    jump_context(&cur_ctx,&old_ctx);
}
/**
 * 回收c栈
 */
Context::~Context(){
    delete[] bp;
}