//
// Created by root on 2020/3/24.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
#ifndef PUMP_TCP_CONNECTOR_HH
#define PUMP_TCP_CONNECTOR_HH

#include <f-stack/ff_api.h>
#include <asm/ioctls.h>
#include <arpa/inet.h>
#include <common/defer.hh>

namespace pump::dpdk::net{
    struct connect_handler{
        common::ncpy_func<bool()> available;
        common::ncpy_func<void(FLOW_ARG(tcp_session)&&)> handle;
    };
    struct tcp_connector:boost::noncopyable{
        struct _data{
            int fd;
            kevent kevset;
            connect_handler waiting_task;
            common::ncpy_func<void(kevent& event)> cb;
        };

        std::shared_ptr<_data> _impl_;


        template <typename _IS_AVALIABLE_FUNC_,typename _HANDLE_FUNC_>
        void schedule(_IS_AVALIABLE_FUNC_&& f1,_HANDLE_FUNC_&& f2){
            _impl_->waiting_task.available=f1;
            _impl_->waiting_task.handle=std::forward<_HANDLE_FUNC_>(f2);
        }

        tcp_connector(tcp_connector&& o)noexcept{
            _impl_=o._impl_;
        }

        explicit
        tcp_connector(int kqfd,int fd){
            _impl_=std::make_shared<_data>();
            _impl_->fd=fd;
            _impl_->cb=[_impl_=_impl_,kqfd](kevent& event){
                EV_SET(&_impl_->kevset, _impl_->fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
                ff_kevent(kqfd, &_impl_->kevset, 1, nullptr, 0, nullptr);
                if(event.flags&EV_EOF){
                    ff_close(_impl_->fd);
                    if(_impl_->waiting_task.available())
                        _impl_->waiting_task.handle(std::make_exception_ptr(std::logic_error("connect exception")));
                }
                else{
                    if(_impl_->waiting_task.available())
                        _impl_->waiting_task.handle(tcp_session(kqfd,_impl_->fd));
                }
            };
        }
    };
    auto
    connect(int kqfd, const char* ip,int port,int timeout){
        sockaddr_in addr;
        addr.sin_port = htons(port);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip);

        tcp_connector connector(kqfd,ff_socket(AF_INET, SOCK_STREAM, 0));
        int on = 1;
        ff_ioctl(connector._impl_->fd,FIONBIO,&on);
        int rtn=ff_connect(connector._impl_->fd,(linux_sockaddr *)&addr, sizeof(addr));
        if(rtn<0 && errno!=EINPROGRESS){
            return reactor::make_imme_flow()
                    .then([connector=std::forward<tcp_connector>(connector)](FLOW_ARG()&& f)->std::variant<int,tcp_session>{
                        throw std::logic_error("connect exception");
                    });
        }
        else{
            EV_SET(&connector._impl_->kevset, connector._impl_->fd, EVFILT_WRITE, EV_ADD, 0, 0, &connector._impl_->cb);
            ff_kevent(kqfd, &connector._impl_->kevset, 1, nullptr, 0, nullptr);
            return reactor::flow_builder<std::variant<int,tcp_session>>::at_schedule
                    (
                            [connector=std::forward<tcp_connector>(connector),timeout](std::shared_ptr<reactor::flow_implent<std::variant<int,tcp_session>>> f)mutable
                            {
                                connector.schedule(
                                        [f](){ return !f->called();},
                                        [connector=std::forward<tcp_connector>(connector),f](FLOW_ARG(tcp_session)&& v)mutable{
                                            ____forward_flow_monostate_exception(v);
                                            auto&& s=std::get<tcp_session>(v);
                                            f->trigge(FLOW_ARG(std::variant<int,tcp_session>)(std::variant<int,tcp_session>(std::forward<tcp_session>(s))));
                                        });
                                timer::_sp_timer_set->add_timer(timeout,[timeout,f](){
                                    f->trigge(FLOW_ARG(std::variant<int,tcp_session>)(std::variant<int,tcp_session>(timeout)));
                                });
                            },
                            reactor::_sp_immediate_runner_
                    );
        }


    }
}
#endif //PUMP_TCP_CONNECTOR_HH

#pragma clang diagnostic pop