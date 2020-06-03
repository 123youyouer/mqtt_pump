//
// Created by root on 2020/5/21.
//

#ifndef PUMP_TASK_PROC_HH
#define PUMP_TASK_PROC_HH

#include <array>
#include <common/cpu.hh>
#include <common/concept.hh>
#include <epoll/net/poller_epoll.hh>
#include <timer/time_set.hh>
#include <epoll/global/global_task_center.hh>


namespace pump::epoll{
    template <typename TIMER>
    concept TIMER_CEPT=     requires (TIMER t){
        {t.nearest_delay(uint64_t())}->common::same_as<std::optional<uint64_t>>;
        {t.handle_timeout(uint64_t())};
    };
    template <typename POLLER>
    concept POLLER_CEPT=    requires (POLLER p){
        {p.wait_event((epoll_event*)(nullptr),int(),int())}->common::same_as<int>;
    };
    template <typename TASKS>
    concept TASKS_CEPT=     requires (TASKS t){
        {t.empty()}->common::same_as<bool>;
        {t.run()};
    };

    template <typename TIMER,typename POLLER,typename TASKS>
    requires TIMER_CEPT<TIMER>&&POLLER_CEPT<POLLER>&&TASKS_CEPT<TASKS>
    auto
    start_proc(sp<TIMER> timer,sp<POLLER> poller,sp<TASKS> task){
        struct __tmp{
            sp<TIMER> _timer;sp<POLLER> _poller;sp<TASKS> _task;
            void run_at(const common::cpu_core& cpu){
                common::pin_this_thread(cpu);
                std::array<epoll_event, 128> eevt{};
                while(true){
                    _timer->handle_timeout(timer::now_tick());
                    auto x=_timer->nearest_delay(timer::now_tick());
                    int wait_timer=-1;
                    if(x)
                        wait_timer= static_cast<int>(x.value());
                    _task->run();
                    if(!_task->empty())
                        wait_timer=0;
                    int cnt=_poller->wait_event(eevt.data(),eevt.size(),wait_timer);
                    if(cnt>0)
                        for(int i=0;i<cnt;++i)
                            (*static_cast<std::function<void(const epoll_event&)>*>(eevt[i].data.ptr))(eevt[i]);
                }
            };
        };
        return __tmp{timer,poller,task};
    }
}
#endif //PUMP_TASK_PROC_HH
