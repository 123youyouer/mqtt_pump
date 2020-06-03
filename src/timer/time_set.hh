//
// Created by null on 20-1-15.
//

#ifndef TIMER_SET_HH
#define TIMER_SET_HH
#include <sys/time.h>
#include <queue>
#include <optional>
#include <boost/noncopyable.hpp>
#include <strings.h>
#include <common/ncpy_func.hh>


namespace pump::timer{
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
    class time_set{
    private:
        std::priority_queue<timer_obj> _q;
    public:
        template <typename _F> void
        add_timer(uint64_t delay,_F&& f){
            _q.emplace(timer_obj(delay,std::forward<_F>(f)));
        }
        bool
        empty(){
            return _q.empty();
        }
        std::optional<uint64_t>
        nearest_delay(uint64_t now){
            if(empty())
                return std::nullopt;
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
    std::shared_ptr<time_set> _sp_timer_set=std::make_shared<time_set>();
}

#endif //TIMER_SET_HH
