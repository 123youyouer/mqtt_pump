//
// Created by root on 2020/5/21.
//

#ifndef PUMP_TASK_PROC_HH
#define PUMP_TASK_PROC_HH

#include <common/cpu.hh>
#include <pump/poller/poller_epoll.hh>
#include <timer/timer_set.hh>

namespace pump::proc{
    struct task_proc_context{
        int wait_start_flag;
        int running_step;
        int epoll_wait_time;
        common::cpu_core cpu;
        std::shared_ptr<poller::poller_epoll> poller;
        std::shared_ptr<timer::timer_set> ts;
    };
    void
    task_proc(task_proc_context& ctx){
        common::pin_this_thread(ctx.cpu);
        std::array<epoll_event, 128> eevt{};
        while(ctx.wait_start_flag){
            auto x=ctx.ts->nearest_delay(timer::now_tick());
            int wait_timer=-1;
            if(x)
                wait_timer= static_cast<int>(x.value());
            ctx.running_step=0;
            ctx.epoll_wait_time=wait_timer;
            int cnt=ctx.poller->wait_event(eevt.data(),eevt.size(),wait_timer);
            ctx.running_step=1;
            ctx.ts->handle_timeout(timer::now_tick());
            if(cnt>0){
                for(int i=0;i<cnt;++i){
                    (*static_cast<std::function<void(const epoll_event&)>*>(eevt[i].data.ptr))(eevt[i]);
                }
            }
        }
    }
}
#endif //PUMP_TASK_PROC_HH
