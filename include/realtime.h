#ifndef __REALTIME_H
#define __REALTIME_H 1

#include <thread>
#include <sched.h>

#define REALTIME_POLICY SCHED_FIFO

void realtime_priority (std::thread & th, int prio);

#endif // __REALTIME_H
