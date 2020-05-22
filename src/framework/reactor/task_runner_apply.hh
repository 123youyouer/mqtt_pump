//
// Created by anyone on 20-2-20.
//

#ifndef PROJECT_TASK_CENTER_HH
#define PROJECT_TASK_CENTER_HH

#include <boost/hana.hpp>
#include <common/g_define.hh>
namespace reactor{
    using namespace boost::hana::literals;
    constexpr auto
            _has_level_=boost::hana::is_valid([](auto t)-> decltype((size_t)decltype(t)::type::_level_){});

    template <typename _T_>
    constexpr auto
    _get_level_(){
        if constexpr (_has_level_(boost::hana::type_c<_T_>)){
            return _T_::_level_;
        }
        else{
            return 9999_c;
        }
    }

    template <class _RUNNER_0,class ..._RUNNER_1>
    struct _task_runner_apply final {
    public:
        _task_runner_apply()= default;
    public:
        //friend class _sort_task_by_priority;
        ALWAYS_INLINE constexpr static auto
        run(_RUNNER_0& r0,_RUNNER_1&... r1){
            _task_runner_apply<_RUNNER_0>::run(r0);
            _task_runner_apply<_RUNNER_1...>::run(r1...);
        }
    };
    template <class _RUNNER_0>
    struct _task_runner_apply<_RUNNER_0> final{
    public:
        _task_runner_apply()= default;
    public:
        //friend class _sort_task_by_priority;

        ALWAYS_INLINE constexpr static auto
        run(_RUNNER_0& r0){
            r0.run_task();
        }
    };

    template <class ..._RUNNER_>
    class _sort_runner_by_priority{
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
            return boost::hana::type_c<_task_runner_apply<typename std::remove_const<typename std::remove_reference<decltype(T)>::type>::type::type...>>;
        };

    public:
        constexpr
        static
        auto impl_sort(){
            typedef decltype(boost::hana::make_tuple(boost::hana::make_pair(_get_level_<_RUNNER_>(),boost::hana::type_c<_RUNNER_>)...)) _inner_types_1;
            typedef decltype(boost::hana::sort(
                    _inner_types_1(),
                    _sort_runner_by_priority<_RUNNER_...>::less)) _inner_types_2;
            typedef decltype(boost::hana::transform(
                    _inner_types_2(),
                    _sort_runner_by_priority<_RUNNER_...>::tran)) _inner_types_3;
            return typename decltype(boost::hana::unpack(
                    _inner_types_3(),
                    _sort_runner_by_priority<_RUNNER_...>::unpack))::type();
        }

        typedef decltype(impl_sort()) typeof_impl_sort_result;
    };
}
#endif //PROJECT_TASK_CENTER_HH
