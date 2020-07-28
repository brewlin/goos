#include "Sysmon.h"
#include <thread>
#include <sys/signal.h>
#include "Proc.h"
vector<pthread_t> Sysmon::_p;
thread Sysmon::_m;
void Sysmon::sighandler(int signo)
{
    regsig();
    return;
}
/**
 * 线程信号处理
 * @param signo
 */
void Sysmon::regsig()
{
//    Sysmon::_p.emplace_back(pthread_self());
    struct sigaction actions;
    sigemptyset(&actions.sa_mask);
    /* 将参数set信号集初始化并清空 */
    actions.sa_flags = -1;
    actions.sa_handler = sighandler;
    /* 设置SIGALRM的处理函数 */
    sigaction(SIGALRM,&actions,NULL);
}
void Sysmon::check()
{
    for(;;){
        for (pthread_t &id : _p) {
            pthread_kill(id, SIGALRM);
        }
    }
    //end the proc
    delete proc;
}
void Sysmon::newm(size_t procn)
{
    regsig();
    if(proc != nullptr)throw "sysmon init failed";
    proc = new Proc(procn);
    if(proc == nullptr) throw "proc init failed";
    _m = thread(check);
}
void Sysmon::wait()
{
    _m.join();
}

