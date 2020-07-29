#include "Sysmon.h"
#include <thread>
#include <sys/signal.h>
#include "Proc.h"
#include "Coroutine.h"
vector<pthread_t> Sysmon::_p;
thread Sysmon::_m;

void Sysmon::preemptPark()
{
    cout << "重新调度" << endl;
    if(GO_ZG(_g) != nullptr){
        GO_ZG(_g)->yield();
    }

}
void Sysmon::sighandler(int signo)
{
    cout << "sighandler: "<< this_thread::get_id() <<endl;
    regsig();
    preemptPark();
    return;
}
/**
 * 线程信号处理
 * @param signo
 */
void Sysmon::regsig()
{
    struct sigaction actions;
    sigemptyset(&actions.sa_mask);
    /* 将参数set信号集初始化并清空 */
    actions.sa_flags = -1;
    actions.sa_handler = sighandler;
    /* 设置SIGALRM的处理函数 */
    sigaction(SIGURG,&actions,NULL);
}
void Sysmon::preemptM(M *m)
{
    //检查周期是否一致
    if(m->tick != GO_FETCH(m->_m,schedtick)){
        //说明周期不一致，更新当前周期
        m->tick = GO_FETCH(m->_m,schedtick);
        return;
    }
    //检查是否超时 上次时间+10ms 如果还小于当前时间
    auto prev = GO_FETCH(m->_m,schedwhen);
    auto now  = chrono::steady_clock::now();
    int timeout = chrono::duration<double,std::milli>(now-prev).count();
    //如果大于10ms 则需要执行抢占
    if(timeout > 10){
        preemptPark()
//            pthread_kill(id, SIGURG);
    }


}
void Sysmon::check()
{
    for(;;){
        this_thread::sleep_for(chrono::milliseconds(100));
        for(M &m : allm){
            preemptM(&m);
//            pthread_kill(id, SIGURG);
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

