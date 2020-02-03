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
#include <logger/logger.hh>

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
        timer::timer_set* ts=timer::all_timer_set[cpu_id];
        while(_all_thread_state_[cpu_id].wait_start_flag<=0){
            sleep(1);
        }
        while(_all_thread_state_[cpu_id].wait_start_flag>0){
            auto x=ts->nearest_delay(timer::now_tick());
            int wait_timer=-1;
            if(x)
                wait_timer= static_cast<int>(x.value());
            _all_thread_state_[cpu_id].running_step=0;
            _all_thread_state_[cpu_id].epoll_wait_time=wait_timer;
            logger::info("***thread wait epoll events at cpu:{0},wait timer :{1}",cpu_id,wait_timer);
            int cnt=poller::_all_pollers[cpu_id]->wait_event(eevt.data(),eevt.size(),wait_timer);
            _all_thread_state_[cpu_id].running_step=1;
            logger::info("***thread recved epoll events at cpu:{0},count :{1}",cpu_id,cnt);
            ts->handle_timeout(timer::now_tick());
            if(cnt>0){
                for(int i=0;i<cnt;++i){
                    (*static_cast<std::function<void(const epoll_event&)>*>(eevt[i].data.ptr))(eevt[i]);
                }
            }

            logger::info("***thread handled epoll events at cpu:{0}",cpu_id);
        }
    }

    template <typename ..._TASK_TYPE>
    auto make_thread(const hw::cpu_core& cpu_id){
        _all_thread_state_[static_cast<int>(cpu_id)].wait_start_flag=0;
        return std::thread(thread_task<_TASK_TYPE...>,cpu_id);
    }
}
#endif //PROJECT_THREAD_BUILDER_HH
