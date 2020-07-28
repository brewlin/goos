#ifndef GOOS_SYSMON_H
#define GOOS_SYSMON_H

#include <thread>
#include <vector>
#include "php_go.h"

class Sysmon
{
public:
    static void sighandler(int signo);
    static void regsig();
    static void check();
    static void wait();
//    bool retake();
    static void newm(size_t proc);
//    bool preemptM();
//    void preemptPark();

public:
    static vector<pthread_t> _p;
    static thread _m;
};



#endif //GOOS_SYSMON_H
