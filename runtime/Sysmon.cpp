#include "Sysmon.h"
#include <thread>
#include <sys/signal.h>
#include "Proc.h"
#include "Coroutine.h"
#include "Log.h"

thread Sysmon::_m;

/**
 * 当前线程的信号处理
 * @param signo
 */
void Sysmon::sighandler(int signo)
{
    //不可靠信号需要重新安装
    regsig();
    unique_lock <mutex> lock(*GO_ZG(_glock),defer_lock);
    lock.lock();
    Coroutine *co = GO_ZG(_g);
    if(co == nullptr)return;
    Debug("receive signal:%d _g:%x co->status:%d Grunnable:%d",signo,co,co->gstatus,Grunnable);
    //判断当前G是否状态正常
    if(co->gstatus == Grunnable){
        //抢占切出
        lock.unlock();
        co->stackpreempt();
    }
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
    sigaction(SIGURG,&actions,NULL);
}
/**
 * 对该线程发送信号，进行抢占调度
 * @param m
 */
void Sysmon::preemptPark(M *m)
{
    //同步两边状态
    m->tick = 0;
    GO_FETCH(m->_m,schedtick) = 0;
    Coroutine *co  = GO_FETCH(m->_m,_g);
    //发起抢占前判断G是否正常
    if(co == nullptr)return;
    Debug("start park preempt: _g:%x co->gstatus:%d Grunnable:%d",co,co->gstatus,Grunnable);
    if(co->gstatus == Grunnable){
        Debug("start send signal:%d",SIGURG);
        pthread_kill(m->tid, SIGURG);
    }

}
/**
 * 都该线程进行周期和持有时间进行检查
 * 如果超过10ms则需要标记为抢占
 * @param m
 */
void Sysmon::preemptM(M *m)
{
    //检查周期是否一致
    if(m->tick != m->G->schedtick){
        Debug("period not consistent m:%x",m->tid);
        m->tick = m->G->schedtick;
        return;
    }
    //检查是否超时 上次时间+10ms 如果还小于当前时间
    auto prev = m->G->schedwhen;
    auto now  = chrono::steady_clock::now();
    int timeout = chrono::duration<double,std::milli>(now-prev).count();
    //如果大于20ms 则需要执行抢占 php初始化比较消耗时间 10ms可能不够
    if(timeout > 20){
        Debug("over 20ms: start call preempt park");
        preemptPark(m);
    }
//    }else{
//        m->tick ++;
//    }
}
/**
 * 监控线程
 */
void Sysmon::monitor()
{
    Debug("sysmon start");
    //等待线程M加载完
    while (proc->threads != proc->start_threads)
        this_thread::sleep_for(chrono::milliseconds(1));
    int pn = 0;
    int bmaxnum = 0;
    for(;;)
    {
        pn = allm.size();
        this_thread::sleep_for(chrono::milliseconds(5));
        int total_n = 0;
        auto now = proc->now;
        for(M &m : allm)
        {
            unique_lock <mutex> lock(*m.G->_glock,try_to_lock);
            if(!lock.owns_lock()){
                Debug("not get lock");
                continue;
            }
            Debug("mid:%x G:%x _g:%d _glock:%x",m.tid,m.G,m.G->_g,m.G->_glock);
            if(m.G->_g != nullptr){
                preemptM(&m);
            }else if(proc->tasks.empty() && m.G->runq->empty()){
                total_n ++;
            }else{
                proc->cond.notify_one();
            }

        }
        double equal = chrono::duration<double,std::milli>(proc->now-now).count();
        //在此次检查线程期间是否 proc->task已更新，如果更新了就不计数
        Debug("check quit: equal: total_n:%d pn:%d ready:%d",total_n,pn,proc->ready);
        if(equal == 0 && total_n == pn && proc->ready)
            bmaxnum ++;
        else bmaxnum = 0;
        if(bmaxnum >= 5)break;
    }
    Debug("sysmon start ending...")
    delete proc;
}
void Sysmon::newm(size_t procn)
{
    regsig();
    assert(proc == nullptr);
    proc = new Proc(procn);
    assert(proc != nullptr);
    _m = thread(monitor);
}
void Sysmon::wait()
{
    _m.join();
}

