#include "Proc.h"
#include "PHPCoroutine.h"
#include "Coroutine.h"
#include "ZendFunction.h"
#include "Sysmon.h"
Proc *proc;

/**
 * 投递一个协程给多个线程调度执行
 * @param ctx
 */
void Proc::gogo(Context* ctx)
{
    unique_lock<mutex> lock(queue_mu);
    this->tasks.emplace(ctx);
    cond.notify_one();
}
/**
 * 重新初始化php全局相关变量
 */
void Proc::preapre_start()
{
    Sysmon::regsig();
    ts_resource(0);
    TSRMLS_CACHE_UPDATE();
    php_request_startup();
}
/**
 * 结束php声明周期，释放相关全局变量
 */
void Proc::prepare_shutdown()
{
    php_request_shutdown((void*)NULL);
    PG(report_memleaks) = 0;
    ts_free_thread();
}
/**
 * 事件循环 去竞争获取投递的协程
 * 并去执行
 */
void Proc::schedule()
{
    for(;;){
        Context* ctx;
        {
            unique_lock<mutex> lock(this->queue_mu);
            this->cond.wait(lock,[this]{
                return this->stop || !this->tasks.empty();
            });

            if(this->stop && this->tasks.empty())
                break;

            ctx = move(this->tasks.front());
            this->tasks.pop();
        }
        Coroutine *co = static_cast<Coroutine *>(ctx->func_data);
        GO_ZG(_g) = co;
        //当前线程分配到一个未初始化的G
        if(co->gstatus == Gidle) co->newproc();
        //恢复被暂停的G
        else co->resume();
        //G运行结束 销毁栈
        if(ctx->is_end) co->close();
        //G被切出来，重新进行调度
        else GO_ZG(rq)->q->put(co);
        //处理切出来的协程
        //TODO :因为实际被让出的协程可能由网络或者时间触发，这里先模拟处理被切出来的协程G
        //实际情况应该有其他如POLLER、timer 等来恢复该协程
        runqget();
        //释放当前产生的G，回收该G相关内存
        free_func();
    }
}
/**
 * 这里需要处理当前被切出的协程，因为php线程隔离的原因
 * 如果当前G被让出，则在恢复的时候依然需要在当前线程M执行
 */
void Proc::runqget()
{
    Runq *rq = static_cast<Runq *>(GO_ZG(rq));
    Coroutine *co;
    while(!rq->q->isEmpty()){
        co = rq->q->pop();
        co->resume();
        if(co->ctx->is_end){
            co->close();
        }
    }
}
/**
 * 初始化创建固定数量的线程
 * @param threads
 */
Proc::Proc(size_t threads):stop(false)
{
    for(size_t i = 0; i < threads ; i ++){
        workers.emplace_back([this,i]
        {
            preapre_start();
            schedule();
            prepare_shutdown();
        });
    }

}
/**
 * 创建的协程一般都是跨线程调度，所以目前回收该内存的方式如下：
 * 从哪创建的协程就从哪回收:针对的是 zend_function*资源
 * 因为在投递协程的时候需要拷贝 zend_function->op_array等信息
 * 而那些信息都是当前线程的AG(mmheap)上申请的，所以目前是在通过creator
 * 传递回去释放
 * @note 该函数执行释放的zend_function 肯定是单签线程创建的协程
 */
void Proc::free_func()
{
    Freeq *free = static_cast<Freeq *>(GO_ZG(q));
    lock_guard<mutex> lock(free->gofq_lock);
    ZendFunction *call;
    while(!free->func_list->isEmpty()){
        call = free->func_list->pop();
        delete call;
    }
}

/**
 * 这里一般是主线程在接受cli的时候会回收当前所有的线程
 */
Proc::~Proc()
{
    {
        unique_lock <mutex> lock(queue_mu);
        stop = true;
    }
    cond.notify_all();
    for (thread &w : workers) {
        free_func();
        w.join();
    }
    //回收一下在主线程中创建的协程内存
    free_func();
}

