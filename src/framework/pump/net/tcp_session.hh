//
// Created by null on 20-1-4.
//

#ifndef PROJECT_SESSION_HH
#define PROJECT_SESSION_HH

#include <variant>
#include <boost/noncopyable.hpp>
#include <pump/net/listener.hh>
#include <pump/net/packet_buffer.hh>
#include <pump/reactor/schedule.hh>
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
            if(send_len>=0)
                _cur+=send_len;
            return send_len;
        }
        bool complated(){return _len==_cur;}
    };
    struct
    tcp_session_exception{
        enum class
        code_s{
            ok          =   0,
            read_error  =   1,
            send_error  =   2
        };
        code_s code;
        tcp_session_exception(const tcp_session_exception& e):code(e.code){}
        tcp_session_exception(tcp_session_exception&& e)noexcept:code(e.code){}
        tcp_session_exception(code_s c):code(c){}
    };
    struct
    tcp_session_read_result:boost::noncopyable{
        linear_ringbuffer_st* p_ringbuffer;
        tcp_session_read_result(linear_ringbuffer_st* p):p_ringbuffer(p){}
    };

    using _read_event_type_=std::variant<std::monostate,net::tcp_session_read_result,net::tcp_session_exception>;

    class session_sendable_event:boost::noncopyable{
    private:
        int _fd;
    public:
        session_sendable_event()= delete;
        explicit session_sendable_event(int fd):_fd(fd){};
        session_sendable_event(session_sendable_event&& o)noexcept:_fd(o._fd){};

    };
}
namespace reactor{
    template <>
    class schedule_able<net::_read_event_type_>{

    public:
        using _running_context_type_=reactor::task_schedule_center<net::_read_event_type_>;
        using _schedule_function_arg_type_=net::_read_event_type_;
    public:
        _running_context_type_* _rc_;
        schedule_able():_rc_(nullptr){}
        void set_schedule_context(_running_context_type_& c){
            _rc_= &c;
        }
    };
    template <>
    struct
    schedule_to<reactor::task_schedule_center<net::_read_event_type_>>{
        static void
        apply(
                reactor::task_schedule_center<net::_read_event_type_>* dst,
                common::ncpy_func<void(net::_read_event_type_&&)>&& f){
            dst->push(std::forward<common::ncpy_func<void(net::_read_event_type_&&)>>(f));
        }
    };
    template <typename _T0>
    struct
    schedule_to<_T0,reactor::task_schedule_center<net::_read_event_type_>>{
        static void
        apply(
                reactor::task_schedule_center<net::_read_event_type_>* dst,
                common::ncpy_func<void(net::_read_event_type_&&)>&& f){
            schedule_to<reactor::task_schedule_center<net::_read_event_type_>>::apply(
                    dst,
                    std::forward<common::ncpy_func<void(net::_read_event_type_&&)>>(f));
        }
    };
    template <>
    class
    schedule_able<net::session_sendable_event>{
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
    struct
    schedule_to<reactor::task_schedule_center<net::session_sendable_event>>{
        static void
        apply(reactor::task_schedule_center<net::session_sendable_event>* dst,common::ncpy_func<void(net::session_sendable_event&&)>&& f){
            dst->push(std::forward<common::ncpy_func<void(net::session_sendable_event&&)>>(f));
        }
    };
    template <typename _T0>
    struct
    schedule_to<_T0,reactor::task_schedule_center<net::session_sendable_event>>{
        static void
        apply(reactor::task_schedule_center<net::session_sendable_event>* dst,common::ncpy_func<void(net::session_sendable_event&&)>&& f){
            schedule_to<reactor::task_schedule_center<net::session_sendable_event>>
            ::apply(dst,std::forward<common::ncpy_func<void(net::session_sendable_event&&)>>(f));
        }
    };
}
namespace net{
    class
    tcp_session final
            : public reactor::task_schedule_center<_read_event_type_>
                    , public reactor::task_schedule_center<net::session_sendable_event>{
        session_data connection_data;
        hw::cpu_core cpu;
        linear_ringbuffer_st read_buf;
        tcp_session_exception::code_s
        recv_all_in_cache(){
            size_t len= static_cast<size_t>(read(connection_data.session_fd,read_buf.write_head(),read_buf.free_size()));
            if(len==-1&&errno==EAGAIN)
                return tcp_session_exception::code_s::ok;
            else if(len<0)
                return tcp_session_exception::code_s::read_error;
            else if(len==0)
                return tcp_session_exception::code_s::read_error;
            else
                read_buf.commit(len);
            return recv_all_in_cache();
        }
        void
        on_read_event(){
            switch(recv_all_in_cache()){
                case tcp_session_exception::code_s::ok:
                {
                    if(read_buf.empty())
                        return;
                    reactor::task_schedule_center<_read_event_type_>::task_type task;
                    if(reactor::task_schedule_center<_read_event_type_>::_q.try_dequeue(task)){
                        _read_event_type_ v;
                        v.emplace<net::tcp_session_read_result>(&this->read_buf);
                        task(std::forward<_read_event_type_>(v));
                    }
                    return;
                }
                case tcp_session_exception::code_s::read_error:
                {
                    close();
                    reactor::task_schedule_center<_read_event_type_>::task_type task;
                    if(reactor::task_schedule_center<_read_event_type_>::_q.try_dequeue(task)){
                        _read_event_type_ v;
                        v.emplace<net::tcp_session_exception>(tcp_session_exception::code_s::read_error);
                        task(std::forward<_read_event_type_>(v));
                    }
                    return;
                }
            }
        }
        void
        on_send_event(){
            reactor::task_schedule_center<net::session_sendable_event>::task_type task;
            if(reactor::task_schedule_center<net::session_sendable_event>::_q.try_dequeue(task))
                task(session_sendable_event(this->connection_data.session_fd));
        }
        void
        on_event(const epoll_event& e){
            if(e.events&EPOLLIN)
                on_read_event();
            if(e.events&EPOLLOUT)
                on_send_event();
        }
    public:
        explicit
        tcp_session(const session_data& session_d):connection_data(session_d),read_buf(){
        }
        void close(){
            poller::_all_pollers[static_cast<int>(cpu)]
                    ->remove_event(connection_data.session_fd);
            ::close(connection_data.session_fd);
        }

        tcp_session&
        start_at(hw::cpu_core cpu){
            this->cpu=cpu;
            int optval = 1;
            ::setsockopt(connection_data.session_fd, SOL_SOCKET, SO_KEEPALIVE,
                         &optval, static_cast<socklen_t>(sizeof optval));
            poller::_all_pollers[static_cast<int>(cpu)]
                    ->add_event(
                            connection_data.session_fd,
                            EPOLLIN|EPOLLOUT|EPOLLET,
                            [this](const epoll_event& e){
                                on_event(e);
                            });
            return *this;
        }
        reactor::flow_builder<linear_ringbuffer_st*>
        wait_packet(size_t len){
            if(read_buf.empty() || read_buf.size()<len)
                return reactor::at_ctx<_read_event_type_>
                        ([this,len](_read_event_type_&& e) {
                            switch(e.index()){
                                case 0:
                                    std::__throw_logic_error("std::monostate");
                                case 1:
                                    return wait_packet(len);
                                case 2:
                                    throw net::tcp_session_exception(std::get<2>(e));

                            }
                        }, *this);

            else
                return reactor::at_cpu(this->cpu,&read_buf);
        }
        reactor::flow_builder<linear_ringbuffer_st*>
        wait_packet(){
            return wait_packet(0);
        }
        reactor::flow_builder<void>
        send_data(send_cache&& cache){
            if(cache.send(connection_data.session_fd)>=0){
                if(cache.complated())
                    return reactor::at_cpu(this->cpu);
                else
                    return reactor::at_ctx<session_sendable_event>
                            ([c = std::forward<send_cache>(cache), this](session_sendable_event &&e)mutable {
                                return this->send_data(std::forward<send_cache>(c));
                            }, *this);
            }
            else{
                switch (errno){
                    case EWOULDBLOCK:
                        return reactor::at_ctx<session_sendable_event>
                                ([c = std::forward<send_cache>(cache), this](session_sendable_event &&e)mutable {
                                    return this->send_data(std::forward<send_cache>(c));
                                }, *this);
                    default:
                        close();
                        std::throw_with_nested(std::forward<net::tcp_session_exception>(tcp_session_exception::code_s::send_error));
                }
            }
        }
        reactor::flow_builder<void>
        send_data(const void* data,int len){
            return send_data(send_cache(static_cast<const char*>(data),len));
        }
    };
}
#endif //PROJECT_SESSION_HH
