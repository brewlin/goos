#ifndef RUNTIME_PROC_H
#define RUNTIME_PROC_H
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include "Context.h"
#include <condition_variable>
#include "php_go.h"
using namespace std;


/**
 * M
 */
struct M
{
    void***     _m;
    pthread_t   tid;
    //标记每次检查抢占的周期，如果需要执行抢占则更新该tick
    //该tick 同时用来标识 在检查期间是否该协程已经发生改变
    int         tick;
    M(){
        _m = (void***)tsrm_get_ls_cache();
        tid = pthread_self();
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
private:
    vector<thread>      workers;
    queue<Context *>    tasks;
    mutex queue_mu;
    condition_variable  cond;
    bool stop;

};

extern Proc *proc;
extern vector<M> allm;

void register_php_runtime();
#endif
