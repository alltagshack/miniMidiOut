#include <thread>
#include <pthread.h>
#include <sched.h>
#include <system_error>

#include "realtime.h"

void realtime_priority (std::thread & th, int prio)
{
    int err = 0;
    pthread_t native = th.native_handle();

    sched_param param{};
    param.sched_priority = prio;
    err = pthread_setschedparam(native, REALTIME_POLICY, &param);

    if (err != 0)
    {
        throw std::system_error(err, std::generic_category(), "pthread_setschedparam failed");
    }
}
