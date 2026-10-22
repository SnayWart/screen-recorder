#ifndef THREAD_UTILS_H
#define THREAD_UTILS_H

#include <sys/resource.h>
#include <sched.h>
#include <pthread.h>
#include <thread>

inline void set_background_thread_affinity_and_priority() {
    setpriority(PRIO_PROCESS, 0, 10);
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    unsigned int n = std::thread::hardware_concurrency();
    if (n >= 8) {
        if (n == 12) {
            CPU_SET(3, &cpuset);
            CPU_SET(4, &cpuset);
            CPU_SET(5, &cpuset);
            CPU_SET(9, &cpuset);
            CPU_SET(10, &cpuset);
            CPU_SET(11, &cpuset);
        } else {
            for (unsigned int i = n / 2; i < n; ++i) {
                CPU_SET(i, &cpuset);
            }
        }
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    }
}

#endif
