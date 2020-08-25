#ifndef GO_COROUTINE_H
#define GO_COROUTINE_H
#include "Context.h"
#include <unordered_map>
#include "Proc.h"
#include "PHPCoroutine.h"
#include <iostream>
#include <mutex>
#include "ZendFunction.h"
using namespace std;

#define PHP_COROUTINE_STACK_SLOT ((int)((ZEND_MM_ALIGNED_SIZE(sizeof(php_sp)) + ZEND_MM_ALIGNED_SIZE(sizeof(zval)) - 1) / ZEND_MM_ALIGNED_SIZE(sizeof(zval))))
#define DEFAULT_PHP_STACK_PAGE_SIZE 1024
/**
 * 协程G状态
 */
enum G_STATUS {Gidle,Grunnable,Preempt,Gdead};

class Coroutine
{
public:
    Context*                                ctx;
    long                                    cid = 0;
    php_sp*                                 stack = nullptr;
    php_sp                                  main_stack = {0};
    void***                                 creator;
    ZendFunction*                           callback;
    int                                     gstatus = Gidle;
    void*                                   php_stack = nullptr;
public:
    Coroutine(run_func func,ZendFunction *args);
    ~Coroutine();
    long run();
    void close();
    void newproc();
    void yield();
    void resume();
    void restore_stack(php_sp *sp);
    void stackpreempt();

};
/**
 * php由于TSRM等线程安全原因，导致内存隔离，所以需要
 * 追踪线程所产生的内存并最终由原线程释放
 */
class Runq
{
public:
    Runq(){q = new QList<Coroutine*>();}
    ~Runq(){if(q) delete q;}
    QList<Coroutine*> *q;
};
/**
 * stack 复用
 */
 struct Stackq{
     Stackq(){q = new QList<Coroutine*>();}
     ~Stackq(){if(q) delete q;}
     QList<Coroutine*> *q;
     Coroutine* get_one(){
         if(q->isEmpty())
            return nullptr;
         Coroutine* co = q->pop();
         //格式化php栈
         memset(co->php_stack,0,DEFAULT_PHP_STACK_PAGE_SIZE);
         //格式化c栈
         memset(co->ctx->bp,0,DEFAULT_STACK);
         co->ctx->reset();
         return co;
     }

 };

#endif //GO_COROUTINE_H
