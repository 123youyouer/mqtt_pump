//
// Created by anyone on 20-2-24.
//

#ifndef PROJECT_SCHDULE_HH
#define PROJECT_SCHDULE_HH

#include <variant>
#include <list>
#include <boost/noncopyable.hpp>
#include <common/g_define.hh>
#include <common/ncpy_func.hh>

#define FLOW_ARG(...) std::variant<std::monostate,std::exception_ptr,##__VA_ARGS__>

namespace engine{
    namespace reactor{

        class flow_runner{
        public:
            ALWAYS_INLINE virtual void
            schedule(common::ncpy_func<void(std::exception_ptr)>&& f)=0;
            ALWAYS_INLINE virtual void
            run()=0;
        };

        class immediate_runner final : public flow_runner{
            ALWAYS_INLINE void
            schedule(common::ncpy_func<void(std::exception_ptr)>&& f)final{
                f(nullptr);
            }
            ALWAYS_INLINE void
            run()final{
            }
        };
        std::shared_ptr<flow_runner> _sp_immediate_runner_(new immediate_runner());

        struct global_task_center : public flow_runner{
            std::list<common::ncpy_func<void(std::exception_ptr)>> waiting_tasks;
            ALWAYS_INLINE void
            run(){
                int len=waiting_tasks.size();
                while(len>0&&!waiting_tasks.empty()){
                    len--;
                    try{
                        (*waiting_tasks.begin())(nullptr);
                    }
                    catch (std::exception& e){
                    }
                    catch (...){
                    }
                    waiting_tasks.pop_front();
                }
            }
            ALWAYS_INLINE void
            schedule(common::ncpy_func<void(std::exception_ptr)>&& f)final{
                waiting_tasks.emplace_back(std::forward<common::ncpy_func<void(std::exception_ptr)>>(f));
            }
        };

        global_task_center _global_task_center_;
        std::shared_ptr<flow_runner> _sp_global_task_center_(new global_task_center());
    }
}
#endif //PROJECT_SCHDULE_HH
