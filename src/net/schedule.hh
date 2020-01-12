//
// Created by null on 20-1-2.
//

#ifndef PROJECT_SCHEDULE_HH
#define PROJECT_SCHEDULE_HH

#include <boost/noncopyable.hpp>
#include <utils/noncopyable_function.hh>
#include <moodycamel/concurrentqueue.h>

namespace reactor{
    template <typename _T>
    struct schedule_center : boost::noncopyable{
    public:
        using task_type=utils::noncopyable_function<void(_T&&)>;
    protected:
        moodycamel::ConcurrentQueue<task_type> _q;
    public:
        void join_schedule_q(task_type&& t){
            _q.enqueue(std::forward<task_type>(t));
        }
    };
}
#endif //PROJECT_SCHEDULE_HH
