#include "Proc.h"
#include "PHPCoroutine.h"
#include "Coroutine.h"
#include "ZendFunction.h"
#include "Sysmon.h"
#include "Log.h"


Proc *proc;
vector<M> allm;
/**
 * 投递一个协程给多个线程调度执行
 * @param ctx
 */
void Proc::gogo(Context* ctx)
{
    assert(!stop);
    unique_lock<mutex> lock(queue_mu);
    now = chrono::steady_clock::now();
    this->tasks.emplace(ctx);
    cond.notify_one();
}
/**
 * 重新初始化php全局相关变量
 */
void Proc::preapre_start()
{
    ts_resource(0);
    TSRMLS_CACHE_UPDATE();
    //将线程索引加入到全局M allm队列中
    M m;
    allm.emplace_back(m);
    php_request_startup();
    //防止线程还没有启动完全 sysmon线程就在读取zend_go_globals*
    start_threads ++;
}
/**
 * 结束php声明周期，释放相关全局变量
 */
void Proc::prepare_shutdown()
{
    queue<Coroutine*> *q = (GO_ZG(free_stack));
    Coroutine *co;
    while(!q->empty()){
        co = move(q->front());
        q->pop();
        delete co;
    }
    zend_string *key;
    void *value = NULL;
//    ZEND_HASH_REVERSE_FOREACH_STR_KEY_VAL(EG(function_table), key, zv) {
//        zend_function *func = (zend_function*)Z_PTR_P(zv);
//        if (func->type == ZEND_INTERNAL_FUNCTION) {
//            continue;
//        }
//        cout << "全局函数名:" << key->val << " " << &func->op_array << endl;
//    } ZEND_HASH_FOREACH_END_DEL();
//    ZEND_HASH_FOREACH_STR_KEY_PTR(CG(function_table), key, value)
//    {
//        cout << "全局函数名:" << key->val << " " << value << endl;
//
//    }
//    ZEND_HASH_FOREACH_END();
    php_request_shutdown((void*)NULL);
    PG(report_memleaks) = 1;
    ts_free_thread();
}
/**
 * 事件循环 去竞争获取投递的协程
 * 并去执行
 */
void Proc::schedule()
{
    Context*   ctx;
    Coroutine* co;
    queue<Coroutine*> *runq = GO_ZG(runq);
    for(;;){
        {
            unique_lock<mutex> lock(this->queue_mu);

            auto res = this->cond.wait_for(lock,chrono::seconds(1)) == cv_status::timeout;
//            this->cond.wait(lock,[this,runq]{
//                return this->stop || !this->tasks.empty() || !runq->q->isEmpty();
//            });
            Debug("G event wait:%d stop:%d tasks.empty:%d q.isEmpty:%d",res,this->stop,this->tasks.empty(),runq->empty());
            if(this->stop || !this->tasks.empty() || !runq->empty()){
                Debug("could get one g");
            }
            else{
                continue;
            }

            if(this->stop && this->tasks.empty() && runq->empty()){
                break;
            }

            if(!this->tasks.empty()){
                ctx = move(this->tasks.front());
                this->tasks.pop();
                co = static_cast<Coroutine *>(ctx->func_data);
            }else{
                co = move(runq->front());
                runq->pop();
            }
        }
        if(co == nullptr){
            Debug("co exception:%ld",co);
            continue;
        }
        Debug("get one G: %ld",co);
        //当前线程分配到一个未初始化的G
        if(co->gstatus == Gidle) co->newproc();
        //恢复被暂停的G
        else co->resume();
        //G运行结束 销毁栈
        if(ctx->is_end)
        {
            Debug("coroutine end: start close");
            co->close();
        }
        //处理切出来的协程
        //TODO :因为实际被让出的协程可能由网络或者其他事件触发，这里先模拟处理被切出来的协程G
        //实际情况应该有其他如POLLER、timer 等来恢复该协程
        //G被切出来，重新进行调度
        else{
            Debug("coroutine yield: start put queue");
            GO_ZG(runq)->push(co);
        }
        //获取本地队列去找到一个可运行的G
        runqget();
    }
}
/**
 * 这里需要处理当前被切出的协程，因为php线程隔离的原因
 * 如果当前G被让出，则在恢复的时候依然需要在当前线程M执行
 */
void Proc::runqget()
{
    queue<Coroutine*> *runq = GO_ZG(runq);
    Coroutine *co;
    while(!runq->empty()){
        co = move(runq->front());
        runq->pop();
        co->resume();
        if(co->ctx->is_end){
            co->close();
        }else{
            runq->push(co);
            break;
        }
    }
}
/**
 * 初始化创建固定数量的线程
 * @param threads
 */
Proc::Proc(size_t threads):stop(false),threads(threads),ready(false)
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
 * 这里一般是主线程在接受cli的时候会回收当前所有的线程
 */
Proc::~Proc()
{
    {
        lock_guard <mutex> lock(queue_mu);
        stop = true;
    }
    cond.notify_all();
    for (thread &w : workers) {
        w.join();
    }
}

