//
// Created by null on 20-1-15.
//

#ifndef PROJECT_TIMER_SET_HH
#define PROJECT_TIMER_SET_HH

#include <boost/noncopyable.hpp>
#include <boost/heap/priority_queue.hpp>
#include <boost/optional.hpp>
#include <sys/time.h>
#include <strings.h>
#include <common/ncpy_func.hh>
#include <engine/reactor/flow.hh>

namespace engine{
    namespace timer{
        uint64_t
        now_tick()
        {
            timeval tv;
            bzero(&tv, sizeof(timeval));
            gettimeofday(&tv, nullptr);
            return static_cast<uint64_t>(tv.tv_sec * 1000 + tv.tv_usec / 1000);
        }
        uint64_t
        compute_timestamp(uint64_t delay){
            return delay+now_tick();
        }
        class timer_obj:boost::noncopyable{
        public:
            uint64_t timestamp;
            common::ncpy_func<void()> cb;
            bool operator < (const timer_obj& o)const{
                return timestamp>o.timestamp;
            }
            template <typename _F>
            explicit timer_obj(uint64_t delay,_F&& f):timestamp(compute_timestamp(delay)),cb(std::forward<_F>(f)){};
            timer_obj(timer_obj&& o)noexcept:timestamp(o.timestamp),cb(std::forward<common::ncpy_func<void()>>(o.cb)){};
            timer_obj& operator = (timer_obj&& o){
                if (this != &o) {
                    this->~timer_obj();
                    new (this) timer_obj(std::forward<timer_obj>(o));
                }
                return *this;
            }
        };
        class timer_set{
        private:
            boost::heap::priority_queue<timer_obj> _q;
        public:
            template <typename _F> void
            add_timer(uint64_t delay,_F&& f){
                _q.emplace(timer_obj(delay,std::forward<_F>(f)));
            }
            bool
            empty(){
                return _q.empty();
            }
            boost::optional<uint64_t>
            nearest_delay(uint64_t now){
                if(empty())
                    return boost::none;
                uint64_t tic=_q.top().timestamp;
                if(tic<=now)
                    return 0;
                return tic-now;
            }
            void
            handle_timeout(uint64_t now){
                while(!_q.empty()){
                    if(_q.top().timestamp>now)
                        return;
                    _q.top().cb();
                    _q.pop();
                }
            }
        };
        std::shared_ptr<timer_set> _sp_timer_set=std::make_shared<timer_set>();
    }
}

#endif //PROJECT_TIMER_SET_HH
