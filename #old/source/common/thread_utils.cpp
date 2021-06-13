#include <thread>
#include <pthread.h>
#include <iostream>
#include <cstring>

#include "thread_utils.h"


namespace ThreadUtils {
    static void setScheduling(std::thread &th, int policy, int priority) {
        #if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
            static sched_param sch_params;
            sch_params.sched_priority = priority;
            if(pthread_setschedparam(th.native_handle(), policy, &sch_params)) {
                std::cerr << "Failed to set Thread scheduling : " << std::strerror(errno) << std::endl;
            }
        #else
            #if defined(_WIN32)
            #else
            #endif
        #endif
    }
}
