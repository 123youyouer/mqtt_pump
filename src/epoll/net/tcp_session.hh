//
// Created by root on 2020/5/26.
//

#ifndef PUMP_EPOLL_TCP_SESSION_HH
#define PUMP_EPOLL_TCP_SESSION_HH

#include <queue>
#include <unistd.h>
#include <sys/socket.h>
#include <epoll/net/poller_epoll.hh>
#include <common/packet_buffer.hh>
#include <net/recv_handler.hh>

namespace pump::epoll::net{
    class tcp_session final : boost::noncopyable {
        int fd;
        sp<poller_epoll> poller;
        sp<common::ringbuffer> read_buf;
        std::queue<pump::net::recv_handler<common::ringbuffer>> waiting_handlers;
        bool
        read_all(){
            size_t len= static_cast<size_t>(read(fd,read_buf->write_head(),read_buf->free_size()));
            if(len==-1 && errno==EAGAIN)
                return true;
            else if(len<0)
                return false;
            else if(len==0)
                return false;
            else
                read_buf->commit(len);
            return read_all();
        }
        ALWAYS_INLINE void
        notify_read(){
            if(waiting_handlers.empty())
                return;
            auto& handler=waiting_handlers.front();
            if(!handler.available())
                return waiting_handlers.pop(),notify_read();
            if(read_buf->empty())
                return;
            if(read_buf->size()<handler.need_len)
                return;
            handler.handle(read_buf);
            waiting_handlers.pop();
        }
        ALWAYS_INLINE void
        notify_close(){
            while(waiting_handlers.empty()){
                auto& handler=waiting_handlers.front();
                if(!handler.available()){
                    waiting_handlers.pop();
                    continue;
                }
                handler.handle(FLOW_ARG(sp<common::ringbuffer>)(std::make_exception_ptr(std::logic_error("session closed"))));
                waiting_handlers.pop();
            }
        }
        ALWAYS_INLINE void
        close(){
            poller->remove_event(fd);
            ::close(fd);
        }
        ALWAYS_INLINE void
        on_read_event(){
            if(read_all())
                notify_read();
            else
                notify_close(),close();
        }
        ALWAYS_INLINE void
        on_send_event(){
        }
    public:
        void
        start(){
            int optval = 1;
            ::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
                         &optval, static_cast<socklen_t>(sizeof optval));
            poller->add_event(
                    fd,
                    EPOLLIN|EPOLLOUT|EPOLLET,
                    [this](const epoll_event& e){
                        if(e.events&EPOLLIN)
                            on_read_event();
                        if(e.events&EPOLLOUT)
                            on_send_event();
                    }
            );
        }
        explicit
        tcp_session(int _fd, sp<poller_epoll>& _poller):fd(_fd),poller(_poller),read_buf(new common::ringbuffer()){
        };
    };
}
#endif //PUMP_TCP_SESSION_HH
