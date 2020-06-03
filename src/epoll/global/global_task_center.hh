//
// Created by root on 2020/5/25.
//

#ifndef PUMP_FLOBAL_HH
#define PUMP_FLOBAL_HH

#include <sys/eventfd.h>
#include <reactor/flow.hh>

namespace pump::epoll::global{
    template <typename T>
    concept _OBSERVER_CEPT_=requires (T t){
        {t.notify_new_task()};
    };
    struct epoll_global_task_center : public pump::reactor::flow_runner{
        std::list<common::ncpy_func<void(FLOW_ARG()&&)>> waiting_tasks;
        ALWAYS_INLINE void
        run()final{
            int len=waiting_tasks.size();
            while(len>0&&!waiting_tasks.empty()){
                len--;
                try{
                    (*waiting_tasks.begin())(FLOW_ARG()(std::monostate()));
                }
                catch (std::exception& e){
                }
                catch (...){
                }
                waiting_tasks.pop_front();
            }
        }
        ALWAYS_INLINE bool
        empty(){
            return waiting_tasks.empty();
        }
        ALWAYS_INLINE void
        schedule(common::ncpy_func<void(FLOW_ARG()&&)>&& f)final{
            waiting_tasks.emplace_back(std::forward<common::ncpy_func<void(FLOW_ARG()&&)>>(f));
        }
    };
}
#endif //PUMP_FLOBAL_HH
