//
// Created by null on 19-9-11.
//

#ifndef CPPMQ_TASK_LOOPER_HH
#define CPPMQ_TASK_LOOPER_HH

#include <boost/hana.hpp>
#include <type_traits>
#include <functional>
#include <iostream>
#include <pump/hw/cpu.hh>
#include "task_channel.hh"


using namespace boost::hana::literals;
namespace reactor{
    namespace loopor{
        template <class _TASK_TYPE_0,class ..._TASK_TYPE_1>
        struct runner final {
        private:
            runner()= default;
        public:
            template <class ..._TASK_TYPE>
            friend class sort_task_by_priority;

            [[gnu::always_inline]][[gnu::hot]]
            constexpr
            static inline auto run(unsigned int& cpu_id){
                std::cout<<1<<std::endl;
                runner<_TASK_TYPE_1...>::run(cpu_id);
            }
        };
        template <class _TASK_TYPE_0>
        struct runner<_TASK_TYPE_0> final{
        private:
            runner()= default;
        public:
            template <class ..._TASK_TYPE>
            friend class sort_task_by_priority;

            [[gnu::always_inline]][[gnu::hot]]
            constexpr
            static inline auto run(unsigned int cpu_id){
                for(int src_id=0;src_id<hw::the_cpu_count;++src_id){
                    auto channel=channel::_all_channels<_TASK_TYPE_0>[src_id][cpu_id];
                    if(channel->_tasks->size_approx()<=0)
                        continue;
                    _TASK_TYPE_0* p=channel->_tasks->peek();
                    std::cout<<"(*p)()"<<std::endl;
                    (*p)();
                    channel->_tasks->pop();
                }
                std::cout<<2<<std::endl;
            }
        };

        template <class ..._TASK_TYPE>
        class sort_task_by_priority{
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
                        sort_task_by_priority<_TASK_TYPE...>::less)) _inner_types_2;
                typedef decltype(boost::hana::transform(
                        _inner_types_2(),
                        sort_task_by_priority<_TASK_TYPE...>::tran)) _inner_types_3;
                return typename decltype(boost::hana::unpack(
                        _inner_types_3(),
                        sort_task_by_priority<_TASK_TYPE...>::unpack))::type();
            }

            typedef decltype(impl_sort()) typeof_impl_sort_result;
        };
    }

    template <class ..._TASK_TYPE>
    constexpr
    auto run(unsigned int cpu_id){
        loopor::sort_task_by_priority<_TASK_TYPE...>::typeof_impl_sort_result::run(cpu_id);
    }

}
#endif //CPPMQ_TASK_LOOPER_HH
