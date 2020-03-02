//
// Created by null on 20-1-5.
//

#ifndef PROJECT_GLOBAL_TASK_SCHEDULE_CENTER_HH
#define PROJECT_GLOBAL_TASK_SCHEDULE_CENTER_HH

#include <common/g_define.hh>
#include <pump/reactor/task.hh>
#include <pump/hw/cpu.hh>
namespace reactor{

    template <typename _TASK_TYPE>
    task_schedule_center<_TASK_TYPE>* global_task_schedule_center[128][128];

    template<typename _TASK_TYPE>
    void init_global_task_schedule_center(int cpu_core_count){
        for(unsigned int src=0;src<cpu_core_count;++src){
            for(unsigned int dst=0;dst<cpu_core_count;++dst){
                global_task_schedule_center<_TASK_TYPE>[src][dst]=new task_schedule_center<_TASK_TYPE>();
            }
        }
    }
    template<typename _T0,typename _T1>
    void init_global_task_schedule_center(int cpu_core_count){
        init_global_task_schedule_center<_T0>(cpu_core_count);
        init_global_task_schedule_center<_T1>(cpu_core_count);
    }
    template<typename _T0,typename _T1,typename _T2,typename ..._TASK_TYPE>
    void init_global_task_schedule_center(int cpu_core_count){
        init_global_task_schedule_center<_T0>(cpu_core_count);
        init_global_task_schedule_center<_T1,_T2,_TASK_TYPE...>(cpu_core_count);
    }

    template <class _TASK_TYPE_0,class ..._TASK_TYPE_1>
    struct runner final {
    private:
        runner()= default;
    public:
        template <class ..._TASK_TYPE>
        friend class _sort_task_by_priority;

        [[gnu::always_inline]][[gnu::hot]]
        constexpr
        static inline auto run(unsigned int cpu_id){
            runner<_TASK_TYPE_0>::run(cpu_id);
            runner<_TASK_TYPE_1...>::run(cpu_id);
        }
    };
    template <class _TASK_TYPE_0>
    struct runner<_TASK_TYPE_0> final{
    private:
        runner()= default;
    public:
        template <class ..._TASK_TYPE>
        friend class _sort_task_by_priority;

        ALWAYS_INLINE
        constexpr
        static auto run(unsigned int cpu_id){
            for(int src_id=0;src_id<hw::the_cpu_count;++src_id)
                global_task_schedule_center<_TASK_TYPE_0>[src_id][cpu_id]->pop_and_run();
        }
    };

    template <class ..._TASK_TYPE>
    class _sort_task_by_priority{
    private:
        constexpr
        static
        auto less=[](auto&& t1,auto&& t2){
            return boost::hana::less(boost::hana::first(t1),boost::hana::first(t2));
        };

        constexpr
        static
        auto tran=[](auto&& o)->auto{
            return boost::hana::second(o);
        };

        constexpr
        static
        auto unpack=[](auto&& ...T)->auto{
            return boost::hana::type_c<runner<typename std::remove_const<typename std::remove_reference<decltype(T)>::type>::type::type...>>;
        };

    public:
        constexpr
        static
        auto impl_sort(){
            typedef decltype(boost::hana::make_tuple(boost::hana::make_pair(_TASK_TYPE::_level,boost::hana::type_c<_TASK_TYPE>)...)) _inner_types_1;
            typedef decltype(boost::hana::sort(
                    _inner_types_1(),
                    _sort_task_by_priority<_TASK_TYPE...>::less)) _inner_types_2;
            typedef decltype(boost::hana::transform(
                    _inner_types_2(),
                    _sort_task_by_priority<_TASK_TYPE...>::tran)) _inner_types_3;
            return typename decltype(boost::hana::unpack(
                    _inner_types_3(),
                    _sort_task_by_priority<_TASK_TYPE...>::unpack))::type();
        }

        typedef decltype(impl_sort()) typeof_impl_sort_result;
    };

    template <class ..._TASK_TYPE>
    void run_task_in_cpu(const hw::cpu_core& cpu){
        _sort_task_by_priority<_TASK_TYPE...>::typeof_impl_sort_result::run(static_cast<unsigned int>(cpu));
    }
}

#endif //PROJECT_GLOBAL_TASK_SCHEDULE_CENTER_HH
