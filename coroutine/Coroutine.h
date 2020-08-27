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

#endif //GO_COROUTINE_H
