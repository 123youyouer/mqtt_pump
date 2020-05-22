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

template <typename ..._ARG_>
ALWAYS_INLINE void
____forward_flow_monostate_exception(const FLOW_ARG(_ARG_...)& v){
    if constexpr (sizeof...(_ARG_)>=1)
        switch (v.index()){
            case 0:     throw std::logic_error("monostate");
            case 1:     std::rethrow_exception(std::get<std::exception_ptr>(v));
            default:    break;
        }
    else
        switch (v.index()){
            case 0:     break;
            case 1:     std::rethrow_exception(std::get<std::exception_ptr>(v));
        }
}

namespace reactor{
    class flow_runner{
    public:
        FLOW_ARG() monostate;
        ALWAYS_INLINE virtual void
        schedule(common::ncpy_func<void(FLOW_ARG()&&)>&& f)=0;
        ALWAYS_INLINE virtual void
        run()=0;
        flow_runner():monostate(std::monostate()){}
    };

    class immediate_runner final : public flow_runner{
        ALWAYS_INLINE void
        schedule(common::ncpy_func<void(FLOW_ARG()&&)>&& f)final{
            f(FLOW_ARG()(std::monostate()));
        }
        ALWAYS_INLINE void
        run()final{
        }
    };
    std::shared_ptr<flow_runner> _sp_immediate_runner_(new immediate_runner());

    struct global_task_center : public flow_runner{
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
        ALWAYS_INLINE void
        schedule(common::ncpy_func<void(FLOW_ARG()&&)>&& f)final{
            waiting_tasks.emplace_back(std::forward<common::ncpy_func<void(FLOW_ARG()&&)>>(f));
        }
    };
    std::shared_ptr<flow_runner> _sp_global_task_center_(new global_task_center());
}
#endif //PROJECT_SCHDULE_HH
