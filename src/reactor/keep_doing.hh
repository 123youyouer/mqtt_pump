//
// Created by root on 2020/3/10.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "InfiniteRecursion"
#ifndef PUMP_KEEP_DOING_HH
#define PUMP_KEEP_DOING_HH

#include <reactor/flow.hh>

namespace pump::reactor{
    template <typename F>
    flow_builder<std::result_of_t<F(FLOW_ARG()&&)>>
    keep_doing(F&& f){
        using _R_=std::result_of_t<F(FLOW_ARG()&&)>;
        return reactor::make_task_flow()
                .then(std::forward<F>(f))
                .then([f=std::forward<F>(f)](FLOW_ARG(_R_)&& v){
                    ____forward_flow_monostate_exception(v);
                    auto r=std::get<_R_>(v);
                    if(!r)
                        return reactor::make_imme_flow(r);
                    else
                        return keep_doing(f);
                });
    }
    template <typename F,typename U>
    flow_builder<std::result_of_t<F(FLOW_ARG()&&)>>
    keep_doing(F&& f,U&& util){
        using _R_=std::result_of_t<F(FLOW_ARG()&&)>;
        return reactor::make_task_flow()
                .then(std::forward<F>(f))
                .then([f=std::forward<F>(f),u=std::forward<U>(util)](FLOW_ARG(_R_)&& v){
                    ____forward_flow_monostate_exception(v);
                    auto r=std::get<_R_>(v);
                    if(!u(r))
                        return reactor::make_imme_flow(r);
                    else
                        return keep_doing(f);
                });
    }
}
#endif //PUMP_KEEP_DOING_HH

#pragma clang diagnostic pop