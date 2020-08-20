#ifndef RUNTIME_PROC_H
#define RUNTIME_PROC_H
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include "Context.h"
#include <condition_variable>
#include "php_go.h"
#include <chrono>
using namespace std;

typedef chrono::time_point<chrono::steady_clock> time_point;
/**
 * M
 */
struct M
{
    void***          _m;
    pthread_t        tid;
    //标记每次检查抢占的周期，如果需要执行抢占则更新该tick
    //该tick 同时用来标识 在检查期间是否该协程已经发生改变
    int              tick;
    //全局线程安全变量
    zend_go_globals* G;
    M()
    {
        _m = (void***)tsrm_get_ls_cache();
        tid = pthread_self();
        G = FETCH_CTX_ALL(_m, go_globals_id, zend_go_globals*);
    }
};


class Proc
{
public:
    Proc(size_t threads);
    ~Proc();
    void                gogo(Context *ctx);
    void                preapre_start();
    void                schedule();
    void                prepare_shutdown();
    static void         free_func();
    void                runqget();
public:
    size_t              threads;
    size_t              start_threads = 0;
    condition_variable  cond;
    queue<Context *>    tasks;
    time_point          now;
private:
    vector<thread>      workers;
    mutex queue_mu;
    bool stop;

};

extern Proc *proc;
extern vector<M> allm;

void register_php_runtime();
#endif
