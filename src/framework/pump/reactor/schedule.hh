//
// Created by null on 19-12-27.
//

#ifndef PROJECT_SCHEDULE_HH
#define PROJECT_SCHEDULE_HH

#include <pump/hw/cpu.hh>
#include <common/ncpy_func.hh>
#include <boost/hana.hpp>
//#include <pump/reactor/task_channel.hh>
#include <pump/reactor/global_task_schedule_center.hh>
#include <pump/reactor/task.hh>
#include <iostream>
#include <pump/poller/poller.hh>
namespace reactor{
    struct _running_context_type_none_{};

    template <typename _V>
    class schedule_able{
    public:
        using _running_context_type_=hw::cpu_core;
        using _schedule_function_arg_type_=void;
        constexpr static _running_context_type_ _default_running_context_=hw::cpu_core::_ANY;
    public:
        _running_context_type_ _rc_;
        schedule_able():_rc_(hw::cpu_core::_ANY){}
        void set_schedule_context(const _running_context_type_& c){
            _rc_=c;
        }
    };

    template <typename ..._T>
    struct schedule_to{
        static void apply(const _T&... t){};
    };

    template <>
    struct schedule_to<hw::cpu_core,hw::cpu_core>{
        static void apply(const hw::cpu_core& _src,const hw::cpu_core& _dst,common::ncpy_func<void()>&& f){
            using task_type=common::ncpy_func<void()>;

            hw::cpu_core src=_src;
            if(src==hw::cpu_core::_ANY)
                src=hw::get_thread_cpu_id();
            hw::cpu_core dst=_dst;
            if(dst==hw::cpu_core::_ANY)
                dst=src;

            reactor::global_task_schedule_center<reactor::sortable_task<task_type,1>>[static_cast<int>(src)][static_cast<int>(dst)]
                    ->push(reactor::sortable_task<task_type,1>(std::forward<common::ncpy_func<void()>>(f)));

            eventfd_write(poller::_all_task_runner_fd[static_cast<int>(dst)],1);
        }
    };

    template <>
    struct schedule_to<hw::cpu_core>{
        static void apply(const hw::cpu_core& dst,common::ncpy_func<void()>&& f){
            schedule_to<hw::cpu_core,hw::cpu_core>::apply(hw::get_thread_cpu_id(),dst,std::forward<common::ncpy_func<void()>>(f));
        }
    };

    template <typename _T0>
    struct schedule_to<_T0,hw::cpu_core>{
        static void apply(const _T0& src,const hw::cpu_core& dst,common::ncpy_func<void()>&& f){
            schedule_to<hw::cpu_core,hw::cpu_core>::apply(hw::get_thread_cpu_id(),dst,std::forward<common::ncpy_func<void()>>(f));
        }
    };
}
#endif //PROJECT_SCHEDULE_HH
