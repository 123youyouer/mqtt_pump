//
// Created by null on 20-1-4.
//

#ifndef PROJECT_SESSION_HH
#define PROJECT_SESSION_HH

#include <boost/noncopyable.hpp>
#include <net/listener.hh>
#include <net/packet_buffer.hh>
#include <reactor/schedule.hh>
namespace net{
    class send_cache:public boost::noncopyable{
    private:
        const char* _buf;
        ssize_t _len;
        ssize_t _cur;
    public:
        send_cache()= delete;
        explicit send_cache(const char* data,ssize_t len):_buf(data),_len(len),_cur(0){}
        send_cache(send_cache&& o)noexcept:_buf(o._buf),_len(o._len),_cur(o._cur){o._buf= nullptr;}
        ssize_t send(int fd){
            ssize_t send_len=write(fd,_buf+_cur,_len-_cur);
            if(send_len<0)
                return send_len;
            _cur+=send_len;
            return send_len;
        }
        bool complated(){return _len==_cur;}
    };
    class session_readable_event:boost::noncopyable{
    private:
        int _fd;
    public:
        session_readable_event()= delete;
        explicit session_readable_event(int fd):_fd(fd){};
    };
    class session_sendable_event:boost::noncopyable{
    private:
        int _fd;
    public:
        session_sendable_event()= delete;
        explicit session_sendable_event(int fd):_fd(fd){};

    };
}
namespace reactor{
    template <>
    class schedule_able<net::session_readable_event>{
    public:
        using _running_context_type_=reactor::task_schedule_center<net::session_readable_event>;
        using _schedule_function_arg_type_=net::session_readable_event;
    public:
        _running_context_type_* _rc_;
        schedule_able():_rc_(nullptr){}
        void set_schedule_context(_running_context_type_& c){
            _rc_= &c;
        }
    };
    template <>
    struct schedule_to<reactor::task_schedule_center<net::session_readable_event>>{
        static void apply(reactor::task_schedule_center<net::session_readable_event>* dst,utils::noncopyable_function<void(net::session_readable_event&&)>&& f){
            dst->push(std::forward<utils::noncopyable_function<void(net::session_readable_event&&)>>(f));
        }
    };
    template <typename _T0>
    struct schedule_to<_T0,reactor::task_schedule_center<net::session_readable_event>>{
        static void apply(reactor::task_schedule_center<net::session_readable_event>* dst,utils::noncopyable_function<void(net::session_readable_event&&)>&& f){
            schedule_to<reactor::task_schedule_center<net::session_readable_event>>::apply(dst,std::forward<utils::noncopyable_function<void(net::session_readable_event&&)>>(f));
        }
    };
    template <>
    class schedule_able<net::session_sendable_event>{
    public:
        using _running_context_type_=reactor::task_schedule_center<net::session_sendable_event>;
        using _schedule_function_arg_type_=net::session_sendable_event;
    public:
        _running_context_type_* _rc_;
        schedule_able():_rc_(nullptr){}
        void set_schedule_context(_running_context_type_& c){
            _rc_= &c;
        }
    };
    template <>
    struct schedule_to<reactor::task_schedule_center<net::session_sendable_event>>{
        static void apply(reactor::task_schedule_center<net::session_sendable_event>* dst,utils::noncopyable_function<void(net::session_sendable_event&&)>&& f){
            dst->push(std::forward<utils::noncopyable_function<void(net::session_sendable_event&&)>>(f));
        }
    };
    template <typename _T0>
    struct schedule_to<_T0,reactor::task_schedule_center<net::session_sendable_event>>{
        static void apply(reactor::task_schedule_center<net::session_sendable_event>* dst,utils::noncopyable_function<void(net::session_sendable_event&&)>&& f){
            schedule_to<reactor::task_schedule_center<net::session_sendable_event>>
            ::apply(dst,std::forward<utils::noncopyable_function<void(net::session_sendable_event&&)>>(f));
        }
    };
}
namespace net{
    class session : public reactor::task_schedule_center<net::session_readable_event>, public reactor::task_schedule_center<net::session_sendable_event>{
        session_data connection_data;
        std::function<void(const epoll_event& e)> _event_hander;
        linear_ringbuffer_st read_buf;
        bool recv_all(){
            size_t len= static_cast<size_t>(read(connection_data.session_fd,read_buf.write_head(),read_buf.free_size()));
            if(len==-1&&errno==EAGAIN)
                return true;
            else if(len<0)
                return false;
            else
                read_buf.commit(len);
            return recv_all();
        }
        void on_read_event(){
            if(!recv_all()){
                close(connection_data.session_fd);
                return;
            }
            if(read_buf.empty())
                return;
            reactor::task_schedule_center<net::session_readable_event>::task_type task;
            if(reactor::task_schedule_center<net::session_readable_event>::_q.try_dequeue(task))
                task(std::forward<net::session_readable_event>(session_readable_event(connection_data.session_fd)));
        }
        void on_send_event(){
            reactor::task_schedule_center<net::session_sendable_event>::task_type task;
            if(reactor::task_schedule_center<net::session_sendable_event>::_q.try_dequeue(task))
                task(session_sendable_event(this->connection_data.session_fd));
        }
        void on_event(const epoll_event& e){
            if(e.events&EPOLLIN)
                on_read_event();
            if(e.events&EPOLLOUT)
                on_send_event();
        }
    public:
        explicit session(const session_data& session_d):connection_data(session_d),read_buf(){
            _event_hander=std::bind(&session::on_event,this,std::placeholders::_1);
        }
        session& start_at(hw::cpu_core cpu){
            int optval = 1;
            ::setsockopt(connection_data.session_fd, SOL_SOCKET, SO_KEEPALIVE,
                         &optval, static_cast<socklen_t>(sizeof optval));
            poller::_all_pollers[static_cast<int>(cpu)]
                    ->add_event(connection_data.session_fd,EPOLLIN|EPOLLOUT|EPOLLET,std::forward<std::function<void(const epoll_event& e)>>(_event_hander));
            return *this;
        }
        auto wait_packet(){
            if(read_buf.empty())
                return reactor::make_flow<session_readable_event>
                        ([this](session_readable_event &&e) {
                            return &read_buf;
                        }, *this);
            else
                return reactor::make_flow([this]() {
                    return &read_buf;
                });
        }
        auto send_data(send_cache&& cache){
            cache.send(connection_data.session_fd);
            if(cache.complated())
                return reactor::make_flow([c = std::forward<send_cache>(cache)]() {
                    return;
                });
            else
                return reactor::make_flow<session_sendable_event>
                        ([c = std::forward<send_cache>(cache), this](session_sendable_event &&e)mutable {
                            return this->send_data(std::forward<send_cache>(c));
                        }, *this);
        }
        auto send_data(const void* data,int len){
            return send_data(send_cache(static_cast<const char*>(data),len));
        }
    };

    void test(){
    }
}
#endif //PROJECT_SESSION_HH
