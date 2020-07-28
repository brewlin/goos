#ifndef GOOS_SYSMON_H
#define GOOS_SYSMON_H

#include <thread>

class Sysmon
{
public:
    bool retake();
    void newm();
    bool preemptM();
    void preemptPark();
    void check();


};

extern pthread_t*


#endif //GOOS_SYSMON_H
