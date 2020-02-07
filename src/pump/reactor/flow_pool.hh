//
// Created by null on 20-2-7.
//

#ifndef PROJECT_FLOW_POOL_HH
#define PROJECT_FLOW_POOL_HH

#include <iostream>
#include <list>
#include <boost/noncopyable.hpp>
#include <utils/g_define.hh>
#include <utils/plf_list.hh>
namespace reactor{
    template<typename _T_>
    struct
    local_flow_pool:boost::noncopyable{
        plf::list<_T_*> flow_pool;
        PUMP_INLINE void
        destroy(_T_ *t){
            flow_pool.push_back(t);
        }
        template <typename ..._A_>
        PUMP_INLINE _T_*
        construct(_A_&&... args){
            if(flow_pool.empty()){
                return new _T_(std::forward<_A_>(args)...);
            }
            else{
                _T_* t=(flow_pool.front());
                flow_pool.pop_front();
                //return t;
                return new(t)_T_(std::forward<_A_>(args)...);
            }
        }
        local_flow_pool(){
        }
    };

    template<typename _T_>
    PUMP_INLINE local_flow_pool<_T_>&
    _local_flow_pool(){
        static thread_local local_flow_pool<_T_>* x=new local_flow_pool<_T_>();
        return *x;
    };
}
#endif //PROJECT_FLOW_POOL_HH
