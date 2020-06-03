//
// Created by null on 19-12-18.
//

#ifndef PROJECT_POLLER_HH
#define PROJECT_POLLER_HH

#include "poller_epoll.hh"
#include <sys/eventfd.h>
#include <iostream>
namespace poller{

    int _all_task_runner_fd[128];

    poller_epoll* _all_pollers[128];

    void init_all_poller(int cpu_core_count){
        for(int i=0;i<cpu_core_count;++i){
            _all_pollers[i]=new poller_epoll;
        }
    }
}
#endif //PROJECT_POLLER_HH
