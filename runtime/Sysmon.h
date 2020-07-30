#ifndef GOOS_SYSMON_H
#define GOOS_SYSMON_H

#include <thread>
#include <vector>
#include "php_go.h"
#include "Proc.h"

class Sysmon
{
public:
    static void sighandler(int signo);
    static void regsig();
    static void monitor();
    static void wait();
    static void newm(size_t proc);
    static void preemptM(M *m);
    static void preemptPark(M *m);

public:
    static thread _m;
};



#endif //GOOS_SYSMON_H
