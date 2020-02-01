//
// Created by null on 20-1-15.
//

#ifndef PROJECT_TIMER_SET_HH
#define PROJECT_TIMER_SET_HH

#include <boost/noncopyable.hpp>
#include <boost/heap/priority_queue.hpp>
#include <boost/optional.hpp>
#include <utils/noncopyable_function.hh>
#include <sys/time.h>
#include <strings.h>
#include <hw/cpu.hh>
#include <reactor/flow.hh>
#include <iostream>
#include <threading/threading_state.hh>

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
    class
    timer_obj:boost::noncopyable{
    public:
        uint64_t timestamp;
        utils::noncopyable_function<void()> cb;
        bool operator < (const timer_obj& o)const{
            return timestamp>o.timestamp;
        }
        template <typename _F>
        explicit timer_obj(uint64_t delay,_F&& f):timestamp(compute_timestamp(delay)),cb(f){};
        timer_obj(timer_obj&& o)noexcept:timestamp(o.timestamp),cb(std::forward<utils::noncopyable_function<void()>>(o.cb)){};
        timer_obj& operator = (timer_obj&& o){
            if (this != &o) {
                this->~timer_obj();
                new (this) timer_obj(std::forward<timer_obj>(o));
            }
            return *this;
        }
    };
    class
    timer_set{
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


    timer_set* all_timer_set[128];

    void
    init_all_timer_set(int cpu_core_count){
        for(int i=0;i<cpu_core_count;++i)
            all_timer_set[i]=new timer_set;
    }

    class
    timer_flow : public reactor::flow<void,void,true>{
    public:
        uint64_t delay;
        void
        submit()override{
            if(this->_rc_==hw::cpu_core::_ANY)
                this->_rc_=hw::get_thread_cpu_id();
            std::cout<<"timer_flow.submit()"<< static_cast<int>(this->_rc_)<<std::endl;
            all_timer_set[static_cast<int>(this->_rc_)]->add_timer(delay,[this](){
                this->active();
            });
            if(threading::_all_thread_state_[static_cast<int>(this->_rc_)].running_step==0)
                if(threading::_all_thread_state_[static_cast<int>(this->_rc_)].epoll_wait_time<0)
                    eventfd_write(poller::_all_task_runner_fd[static_cast<int>(this->_rc_)],1);
        }
        explicit
        timer_flow(uint64_t _delay):delay(_delay),reactor::flow<void,void,true>([](){}){}
    };

    reactor::flow_builder<void>
    delay(uint64_t tick){
        return reactor::flow_builder(new timer_flow(tick));
    }
}
#endif //PROJECT_TIMER_SET_HH
