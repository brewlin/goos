#include "Coroutine.h"
#include "Log.h"
/**
 * 创建一个G 并初始化C栈，并标记当前G所属的线程
 * @param func
 * @param args
 */
Coroutine::Coroutine(run_func func,ZendFunction *args)
{
    callback = args;
    ctx = new Context(func,static_cast<void *>(this));
    creator = args->creator;
}
/**
 * 获取一个G
 * 从本地空闲free_stack中获取一个
 * @return Corouine*
 */
Coroutine* Coroutine::getg(ZendFunction* fn)
{
    Coroutine *co = nullptr;
    queue<Coroutine*> *free = GO_ZG(free_stack);
    Debug("get g free_list:%x size:%ld",free,free->size());
    if(!free->empty()){
        co = move(free->front());
        free->pop();
        //格式化php栈
        memset(co->php_stack,0,DEFAULT_PHP_STACK_PAGE_SIZE);
        //格式化c栈
        memset(co->ctx->bp,0,DEFAULT_STACK);
        co->ctx->reset();
        co->callback = fn;
        co->gstatus = Gidle;
        co->creator = fn->creator;
    }else{
        co = new Coroutine(PHPCoroutine::run,fn);
    }
    co->gstatus = Gidle;
    return co;
}
/**
 * 投递到调度到其他线程CPU中去执行
 * @return
 */
long Coroutine::run()
{
    Debug("G run: start push g to global queue g:%x ctx:%x", this, ctx);
//    投递到 proc 线程去执行该协程
//    if(proc == nullptr)Error("proc not init");
//    proc->gogo(ctx);
    return 1;
}
/**
 * 1.切出当前协程
 * 2.保存当前协程栈状态
 * 3.恢复之前的状态
 */
void Coroutine::yield()
{
    Debug("yield G: g:%x",this);
    PHPCoroutine::save_stack(&main_stack);
    restore_stack(stack);
    {
//        unique_lock <mutex> lock(*GO_ZG(_glock));
        Debug("_glock:%x",GO_ZG(_glock));
        GO_ZG(_g) = nullptr;
        //每次切换出去时需要更新tick 和时间
        GO_ZG(schedwhen) = chrono::steady_clock::now();
        GO_ZG(schedtick) = 0;
    }
    gstatus = Gdead;
    ctx->swap_out();
}
/**
 * 1.表示当前线程分配到一个未初始化的G
 * 2.需要初始化栈，运行环境
 * 3.更新为Grunnable 并执行该用户态函数
 */
void Coroutine::newproc()
{
    Debug("run new G: g:%x",this);
    callback->is_new = 0;
    callback->prepare_functions(this);
    PHPCoroutine::save_stack(&main_stack);
    {
        unique_lock <mutex> lock(*GO_ZG(_glock),defer_lock);
        if(!lock.try_lock())
            Debug(" not get the lock");
        Debug("_glock:%x",GO_ZG(_glock));
        GO_ZG(_g) = this;
        //每次切入时出去时需要更新tick 和时间
        GO_ZG(schedwhen) = chrono::steady_clock::now();
        GO_ZG(schedtick) += 1;
    }
    gstatus = Grunnable;
    ctx->swap_in();
}
/**
 * 1.当前线程被分配一个已经初始化且被切出的协程
 * 2.恢复G之前的状态
 * 3. 存储当前的状态
 */
void Coroutine::resume()
{
    Debug("resume G: mark Grunnable g:%x",this);
    restore_stack(&main_stack);
    PHPCoroutine::save_stack(stack);
    {
        unique_lock <mutex> lock(*GO_ZG(_glock),defer_lock);
        if(!lock.try_lock())
            Debug(" not get the lock");
        Debug("_glock:%x",GO_ZG(_glock));
        GO_ZG(_g) = this;
        //每次切入时出去时需要更新tick 和时间
        GO_ZG(schedwhen) = chrono::steady_clock::now();
        GO_ZG(schedtick) += 1;
    }
    gstatus = Grunnable;
    ctx->swap_in();
}
/**
 * stack preempt
 */
void Coroutine::stackpreempt()
{
    Debug("exec stack preempt:%x gstatus:%d ctx->is_end:%d",this,gstatus,ctx->is_end);
//    gstatus = Preempt;
    yield();

}
/**
 * 切换php栈,在每次切换c栈的同时也需要切换该栈
 * @param sp
 */
void Coroutine::restore_stack(php_sp *sp)
{
    if(sp == nullptr)return;
    EG(vm_stack_top) = sp->vm_stack_top;
    EG(vm_stack_end) = sp->vm_stack_end;
    EG(vm_stack) = sp->vm_stack;
    EG(vm_stack_page_size) = sp->vm_stack_page_size;
    EG(current_execute_data) = sp->execute_data;
}
/**
 * 回收php栈
 * 回收C栈
 */
void Coroutine::close()
{
    zend_vm_stack stack = EG(vm_stack);
//    free(stack);
    php_stack = stack;
    GO_ZG(free_stack)->push(this);
    restore_stack(&main_stack);
//    delete ctx;
//    delete this;
}
Coroutine::~Coroutine()
{
    free(php_stack);
    delete ctx;
}
