#include "PHPCoroutine.h"
#include "Coroutine.h"
#include "ZendFunction.h"
#include <queue>

/**
 * 创建一个协程G运行
 * @param call
 * @return
 */
long PHPCoroutine::go(zend_function *func,zval *argv,uint32_t argc)
{
    ZendFunction *call = new ZendFunction(func,argv,argc);
    Coroutine *ctx;
    //TODO:在主线程时 无本地队列！

    queue<Coroutine*> *q = GO_ZG(free_stack);
    cout << "q:" << q << " size:" << q->size() <<endl;
    if(!q->empty()){
        cout << "fetch" <<endl;
        ctx = move(q->front());
        q->pop();
        //格式化php栈
        memset(ctx->php_stack,0,DEFAULT_PHP_STACK_PAGE_SIZE);
        //格式化c栈
        memset(ctx->ctx->bp,0,DEFAULT_STACK);
        ctx->ctx->reset();
        ctx->callback = call;
    }else{
        cout << "new" <<endl;
        ctx = new Coroutine(run, call);
    }
    return ctx->run();
}
/**
 * 保存当前PHP栈,在每次切换、切出时基本要保存当前php栈
 */
void PHPCoroutine::save_stack(php_sp *sp)
{
    sp->vm_stack_top = EG(vm_stack_top);
    sp->vm_stack_end = EG(vm_stack_end);
    sp->vm_stack = EG(vm_stack);
    sp->vm_stack_page_size = EG(vm_stack_page_size);
    sp->execute_data = EG(current_execute_data);
}
/**
 * 为当前要执行的协程G 从堆上申请一段 空间作为php栈
 */
void PHPCoroutine::init_stack(Coroutine* co)
{
    uint32_t size = DEFAULT_PHP_STACK_PAGE_SIZE;
    zend_vm_stack page;
    //从堆上申请内存  来模拟栈
    if(co->php_stack != nullptr)
        page = (zend_vm_stack)co->php_stack;
    else
        page = (zend_vm_stack)malloc(size);
    //栈顶
    page->top = ZEND_VM_STACK_ELEMENTS(page);
    //end 用来表示栈的边界
    page->end = (zval *)((char *)page + size);
    page->prev = NULL;
    //表示修改现在的php栈，指向我们申请的堆栈
    EG(vm_stack) = page;
    EG(vm_stack)->top ++ ;
    EG(vm_stack_top) = EG(vm_stack)->top;
    EG(vm_stack_end) = EG(vm_stack)->end;
    EG(vm_stack_page_size) = size;
}

/**
 *
 * @param args
 */
void PHPCoroutine::run(void *args)
{
    zval _retval,*retval = &_retval;
    Coroutine *co = static_cast<Coroutine *>(args);
    //获取php call 传入的回调函数
    int i;
    ZendFunction *callback = static_cast<ZendFunction*>(co->callback);
    zend_function *func = callback->func;
    zval *argv = callback->argv;
    int argc = callback->argc;
    //php栈
    php_sp *sp;
    zend_execute_data *call;
    //获取一个新的php栈
    init_stack(co);
    call = (zend_execute_data *)(EG(vm_stack_top));
    sp = (php_sp *) EG(vm_stack_top);
    EG(vm_stack_top) = (zval *)((char *)call + PHP_COROUTINE_STACK_SLOT * sizeof(zval));
    call = zend_vm_stack_push_call_frame(
            ZEND_CALL_TOP_FUNCTION | ZEND_CALL_ALLOCATED,
//            func,argc,fci_cache.called_scope,fci_cache.object
            func,argc,NULL,NULL
    );
    //拷贝用户传入的参数
    for(i = 0; i < argc; ++ i ){
        zval *param;
        zval *arg = &argv[i];
        param = ZEND_CALL_ARG(call,i + 1);
        ZVAL_COPY(param,arg);
    }
    call->symbol_table = NULL;
    //表示将用户传入的闭包函数 设置到php栈上 表示进行函数调用
    EG(current_execute_data) = call;
    save_stack(sp);
    //为当前的协程设置栈
    sp->co = co;
    co->stack = sp;

    GO_ZG(_g) =  co;
    //每次切入时出去时需要更新tick 和时间
    GO_ZG(schedwhen) = chrono::steady_clock::now();
    GO_ZG(schedtick) += 1;
    co->gstatus = Grunnable;
    //把当前协程栈信息保存到task里面
    if(func->type == ZEND_USER_FUNCTION){
        ZVAL_UNDEF(retval);
        EG(current_execute_data) = NULL;
        zend_init_func_execute_data(call,&func->op_array,retval);
        zend_execute_ex(EG(current_execute_data));
    }
    co->gstatus = Gdead;
    GO_ZG(_g) = nullptr;
    //释放co
    delete co->callback;
    zval_ptr_dtor(retval);
}


