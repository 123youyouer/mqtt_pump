//
// Created by null on 19-11-21.
//

#ifndef PROJECT_TASK_HH
#define PROJECT_TASK_HH

#include <boost/hana.hpp>
#include <boost/noncopyable.hpp>
#include <common/ncpy_func.hh>
#include <moodycamel/concurrentqueue.h>
#include <moodycamel/readerwriterqueue.h>

using namespace boost::hana::literals;

namespace reactor{
    template <typename _T>
    struct task_schedule_center : boost::noncopyable{
    public:
        using task_type=common::ncpy_func<void(_T&&)>;
    public:
        moodycamel::ConcurrentQueue<task_type> _q;
        void push(task_type&& e){
            _q.enqueue(std::forward<task_type>(e));
        }
        size_t size(){
            return _q.size_approx();
        }
    };


    template <typename _T, int lvl>
    class sortable_task : public _T{
    public:
        constexpr static auto _level=boost::hana::integral_constant<int,lvl>{};
        explicit sortable_task(_T&& _t):_T(std::forward<_T>(_t)){}
    };

    template <typename _T, int lvl>
    struct task_schedule_center<sortable_task<_T,lvl>> : boost::noncopyable{
    private:
        void run_one(){
            if(_q.size_approx()<=0)
                return;
            (*_q.peek())();
            _q.pop();
        }
    public:
        using task_type=sortable_task<_T,lvl>;
    public:
        moodycamel::ReaderWriterQueue<task_type,1024> _q;
        void pop_and_run()noexcept{
            try{
                size_t this_round_count=_q.size_approx();
                for(size_t i=0;i<this_round_count;++i)
                    run_one();
            }
            catch (...){
            }
        }
        void push(task_type&& e){
            _q.enqueue(std::forward<task_type>(e));
        }
        size_t size(){
            return _q.size_approx();
        }
    };

    template<class _P>
    class task_level_1 : public _P{
    public:
        constexpr static auto _level=1_c;
        explicit task_level_1(_P&& _p):_P(std::forward<_P>(_p)){}
    };
    template<class _P>
    class task_level_2 : public _P{
    public:
        constexpr static auto _level=2_c;
    };
}
#endif //PROJECT_TASK_HH
