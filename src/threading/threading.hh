//
// Created by null on 19-12-15.
//

#ifndef PROJECT_THREAD_BUILDER_HH
#define PROJECT_THREAD_BUILDER_HH

#include <boost/hana.hpp>
#include <thread>
#include <poller/poller.hh>
#include <reactor/global_task_schedule_center.hh>
#include <timer/timer_set.hh>
#include <threading/threading_state.hh>
#include <utils/spinlock.hh>
#include <bits/types/siginfo_t.h>
#include <mutex>
#include <csignal>

namespace threading{
    template <typename ..._TASK_TYPE>
    void thread_task(const hw::cpu_core& running_cpu_id){
        auto cpu_id=static_cast<unsigned int>(running_cpu_id);
        hw::pin_this_thread(cpu_id);
        poller::_all_task_runner_fd[cpu_id]=eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        poller::_all_pollers[cpu_id]->add_event(poller::_all_task_runner_fd[cpu_id],EPOLLIN|EPOLLET,[cpu=cpu_id](const epoll_event&){
            reactor::run_task_in_cpu<_TASK_TYPE...>(hw::cpu_core(cpu));
        });
        std::array<epoll_event, 128> eevt;
        bool forever=true;
        timer::timer_set* ts=timer::all_timer_set[cpu_id];
        while(forever){
            std::cout<<"***********************"<<cpu_id<<" start**********************************"<<std::endl;
            auto x=ts->nearest_delay(timer::now_tick());
            int wait_timer=-1;
            if(x)
                wait_timer= static_cast<int>(x.value());
            std::cout<<"***********************"<<"wait timer"<<":"<<wait_timer<<"  join**********************************"<<cpu_id<<std::endl;
            _all_thread_state_[cpu_id].running_step=0;
            _all_thread_state_[cpu_id].epoll_wait_time=wait_timer;
            int cnt=poller::_all_pollers[cpu_id]->wait_event(eevt.data(),eevt.size(),wait_timer);
            _all_thread_state_[cpu_id].running_step=1;
            std::cout<<"***********************"<<cpu_id<<":"<<cnt<<"  join**********************************"<<std::endl;
            ts->handle_timeout(timer::now_tick());
            if(cnt>0){
                for(int i=0;i<cnt;++i){
                    (*static_cast<std::function<void(const epoll_event&)>*>(eevt[i].data.ptr))(eevt[i]);
                }
            }

            std::cout<<"***********************"<<cpu_id<<"   end**********************************"<<std::endl;
        }
    }

    template <typename ..._TASK_TYPE>
    auto make_thread(const hw::cpu_core& cpu_id){
        return std::thread(thread_task<_TASK_TYPE...>,cpu_id);
    }
}
#endif //PROJECT_THREAD_BUILDER_HH
