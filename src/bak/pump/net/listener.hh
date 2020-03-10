//
// Created by null on 19-12-18.
//

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#ifndef PROJECT_LISTENER_HH
#define PROJECT_LISTENER_HH

#include <moodycamel/concurrentqueue.h>
#include "../poller/poller_epoll.hh"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <iostream>
//#include <pump/reactor/flow.hh>
#include "../reactor/schedule.hh"
#include "../logger/logger.hh"
namespace net{
    struct connection_data{
        int session_fd;
        struct sockaddr peer_addr;
        explicit connection_data(int fd,const sockaddr& addr):session_fd(fd),peer_addr(addr){
        }
    };

    struct session_data{
        int session_fd;
        struct sockaddr peer_addr;
        explicit session_data(int fd,const sockaddr& addr):session_fd(fd),peer_addr(addr){
        }
    };
}

namespace reactor{
    template <>
    class schedule_able<net::connection_data>{
    public:
        using _running_context_type_=reactor::task_schedule_center<net::connection_data>;
        using _schedule_function_arg_type_=net::connection_data;
    public:
        _running_context_type_* _rc_;
        schedule_able():_rc_(nullptr){}
        void set_schedule_context(_running_context_type_& c){
            _rc_= &c;
        }
    };
    template <>
    struct schedule_to<reactor::task_schedule_center<net::connection_data>>{
        static void apply(reactor::task_schedule_center<net::connection_data>* dst,common::ncpy_func<void(net::connection_data&&)>&& f){
            dst->push(std::forward<common::ncpy_func<void(net::connection_data&&)>>(f));
        }
    };
    template <typename _T0>
    struct schedule_to<_T0,reactor::task_schedule_center<net::connection_data>>{
        static void apply(reactor::task_schedule_center<net::connection_data>* dst,common::ncpy_func<void(net::connection_data&&)>&& f){
            schedule_to<reactor::task_schedule_center<net::connection_data>>::apply(dst,std::forward<common::ncpy_func<void(net::connection_data&&)>>(f));
        }
    };
}

namespace net{
    template <uint16_t _PORT_>
    class
    listener final : public reactor::task_schedule_center<net::connection_data>{
    private:
        int listen_fd;
        struct sockaddr_in server_addr;
    private:
        void
        accept(const epoll_event& e){
            sockaddr addr;
            socklen_t len= sizeof(addr);
            logger::info("PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP");
            int session_fd=accept4(this->listen_fd,&addr,&len,SOCK_NONBLOCK | SOCK_CLOEXEC);
            if(session_fd>0){
                reactor::task_schedule_center<net::connection_data>::task_type task;
                if(_q.try_dequeue(task)){
                    task(connection_data(session_fd,addr));
                }
                else{
                    close(session_fd);
                }
            }
        }
        explicit
        listener()
                :reactor::task_schedule_center<net::connection_data>(){
        };
        void
        stop(){
            close(listen_fd);
        }
    public:
        static listener<_PORT_> instance;
        listener<_PORT_>&
        start_at(hw::cpu_core cpu){
            listen_fd = socket(AF_INET, SOCK_STREAM, 0);
            bzero(&server_addr, sizeof(server_addr));
            server_addr.sin_family         = AF_INET;
            server_addr.sin_addr.s_addr    = htonl(INADDR_ANY);
            server_addr.sin_port           = htons(_PORT_);

            bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
            fcntl(listen_fd, F_SETFL, fcntl(listen_fd, F_GETFL)|O_NONBLOCK);

            int optval;
            optval=1;
            setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
                       &optval, static_cast<socklen_t>(sizeof optval));
            optval=1;
            setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT,
                       &optval, static_cast<socklen_t>(sizeof optval));

            if(0==listen(listen_fd, 1024)){
                poller::_all_pollers[static_cast<int>(cpu)]->add_event(
                        listen_fd,
                        EPOLLIN|EPOLLET,
                        [this](const epoll_event& e){
                            accept(e);
                        });
                return *this;
            }
            else{
                return *this;
            }

        }
    };

    template <uint16_t _PORT_>
    listener<_PORT_> listener<_PORT_>::instance;


    template <uint16_t _PORT_>
    auto
    wait_connect(listener<_PORT_>& l){
        return reactor::at_ctx<connection_data>
                ([](connection_data &&d) {
                    return session_data(d.session_fd, d.peer_addr);
                }, l);
    }
}

#endif //PROJECT_LISTENER_HH

#pragma clang diagnostic pop