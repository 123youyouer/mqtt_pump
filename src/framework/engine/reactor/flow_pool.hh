//
// Created by null on 20-2-7.
//

#ifndef PROJECT_FLOW_POOL_HH
#define PROJECT_FLOW_POOL_HH

#include <iostream>
#include <list>
#include <boost/noncopyable.hpp>
#include <boost/hana.hpp>
#include <common/g_define.hh>
#include <common/plf_list.hh>

namespace engine{
    namespace reactor{
        template <typename _T_,typename ..._A_>
        constexpr auto
        maybe_reinit=boost::hana::sfinae([](auto&& t,_A_&... a)->decltype(t->reinit(std::forward<_A_>(a)...)){
            return t->reinit(std::forward<_A_>(a)...);
        });

        template <typename _T_,typename ..._A_>
        constexpr _T_*
        reinit_or_create(_T_* t,_A_&&... a){
            return (maybe_reinit<_T_,_A_...>(t,std::forward<_A_>(a)...).value_or(new(t)_T_(std::forward<_A_>(a)...)));
        }

        template<typename _T_>
        struct
        local_flow_pool:boost::noncopyable{
            plf::list<_T_*> flow_pool;
            ALWAYS_INLINE void
            destroy(_T_ *t){
                flow_pool.push_back(t);
            }

            template <typename ..._A_>
            ALWAYS_INLINE _T_*
            construct(_A_&&... args){
                if(flow_pool.empty()){
                    return new _T_(std::forward<_A_>(args)...);
                }
                else{
                    _T_* t=(flow_pool.front());
                    flow_pool.pop_front();
                    return reinit_or_create<_T_,_A_...>(t,std::forward<_A_>(args)...);
                }
            }
            local_flow_pool(){
            }
        };

        template<typename _T_>
        ALWAYS_INLINE local_flow_pool<_T_>&
        _local_flow_pool(){
            static thread_local local_flow_pool<_T_>* x=new local_flow_pool<_T_>();
            return *x;
        };
    }
}

#endif //PROJECT_FLOW_POOL_HH
