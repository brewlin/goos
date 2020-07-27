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
void register_php_runtime();
#endif
