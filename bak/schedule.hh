//
// Created by null on 19-12-1.
//

#ifndef PROJECT_SCHDULE_HH
#define PROJECT_SCHDULE_HH

#include "utils/noncopyable_function.hh"
#include "reactor/task_channel.hh"
#include "reactor/task.hh"
#include "pump/poller/poller.hh"
#include <iostream>

namespace reactor{
    namespace internal{
        [[gnu::always_inline]][[gnu::hot]]
        inline
        void schdule(utils::noncopyable_function<void()>&& f,int src_cpu, int dst_cpu){
            using task_type=utils::noncopyable_function<void()>;
            reactor::channel::_all_channels<reactor::task_level_1<task_type>>[src_cpu][dst_cpu]->_tasks->emplace(
                    reactor::task_level_1<task_type>(std::forward<utils::noncopyable_function<void()>>(f))
            );
            std::cout<<"dst_cpu="<<dst_cpu<<std::endl;
            int x=eventfd_write(poller::_all_event_fd[dst_cpu],1);
            std::cout<<"eventfd_write="<<reactor::channel::_all_channels<reactor::task_level_1<task_type>>[src_cpu][dst_cpu]->_tasks->size_approx()<<std::endl;
        }
    }
}


#endif //PROJECT_SCHDULE_HH
