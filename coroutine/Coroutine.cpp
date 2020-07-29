#include "Coroutine.h"
/**
 * 创建一个G 并初始化C栈，并标记当前G所属的线程
 * @param func
 * @param args
 */
Coroutine::Coroutine(run_func func,ZendFunction *args)
{
    callback = args;
    ctx = new Context(func,static_cast<void *>(this));
    creator = (void***)tsrm_get_ls_cache();
}
/**
 * 投递到调度到其他线程CPU中去执行
 * @return
 */
long Coroutine::run()
{
    //投递到 proc 线程去执行该协程
    if(proc == nullptr){
        cout << "未初始化线程" <<endl;
        throw "未初始化线程";
    }
    proc->gogo(ctx);
    return 1;
}
/**
 * 1.切出当前协程
 * 2.保存当前协程栈状态
 * 3.恢复之前的状态
 */
void Coroutine::yield()
{
    PHPCoroutine::save_stack(&main_stack);
    restore_stack(stack);
    ctx->swap_out();
    GO_ZG(_g) = nullptr;
    //每次切换出去时需要更新tick 和时间
    GO_ZG(schedwhen) = chrono::steady_clock::now();
    GO_ZG(schedtick) = 0;
}
/**
 * 1.表示当前线程分配到一个未初始化的G
 * 2.需要初始化栈，运行环境
 * 3.更新为Grunnable 并执行该用户态函数
 */
void Coroutine::newproc()
{
    ZendFunction::prepare_functions(this);
    PHPCoroutine::save_stack(&main_stack);
    GO_ZG(_g) = this;
    //每次切入时出去时需要更新tick 和时间
    GO_ZG(schedwhen) = chrono::steady_clock::now();
    GO_ZG(schedtick) += 1;

    ctx->swap_in();
    gstatus = Grunnable;
}
/**
 * 1.当前线程被分配一个已经初始化且被切出的协程
 * 2.恢复G之前的状态
 * 3. 存储当前的状态
 */
void Coroutine::resume()
{
    restore_stack(&main_stack);
    PHPCoroutine::save_stack(stack);

    //每次切入时出去时需要更新tick 和时间
    GO_ZG(schedwhen) = chrono::steady_clock::now();
    GO_ZG(schedtick) += 1;
    GO_ZG(_g) = this;
    ctx->swap_in();
}
/**
 * 切换php栈,在每次切换c栈的同时也需要切换该栈
 * @param sp
 */
void Coroutine::restore_stack(php_sp *sp) {
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
    efree(stack);
    restore_stack(&main_stack);
    delete ctx;
    delete this;
}
