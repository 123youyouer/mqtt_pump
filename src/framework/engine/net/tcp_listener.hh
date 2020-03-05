//
// Created by anyone on 20-2-21.
//

#ifndef PROJECT_LISTENER_HH
#define PROJECT_LISTENER_HH

#include <f-stack/ff_api.h>
#include <cstring>
#include <common/ncpy_func.hh>
#include <engine/reactor/flow.hh>
#include <engine/net/tcp_session.hh>
namespace engine::net{
    struct tcp_listener:boost::noncopyable{
        struct data{
            int socket_fd;
            kevent kevset;
            sockaddr_in addr;
            std::list<common::ncpy_func<void(tcp_session)>> waiting_tasks;
            std::list<tcp_session> waiting_connections;
            common::ncpy_func<void(kevent& event)> cb;
            explicit
            data():socket_fd(0){
                explicit_bzero(&addr, sizeof(addr));
            }
        };
        std::shared_ptr<data> _data;
        void schedule(common::ncpy_func<void(tcp_session&&)>&& f){
            if(_data->waiting_connections.empty()){
                std::cout<<"start listen"<<std::endl;
                _data->waiting_tasks.emplace_back(std::forward<common::ncpy_func<void(tcp_session&&)>>(f));
            }
            else{
                f(std::forward<tcp_session>(_data->waiting_connections.front()));
                _data->waiting_connections.pop_front();
            }

        }
        explicit
        tcp_listener(int kqfd,uint16_t port){
            int max_size=512;
            _data=std::make_shared<data>();
            _data->socket_fd=ff_socket(AF_INET, SOCK_STREAM, 0);
            _data->addr.sin_family = AF_INET;
            _data->addr.sin_port = htons(port);
            _data->addr.sin_addr.s_addr = htonl(INADDR_ANY);
            _data->cb=[_data=_data,kqfd](kevent& event){
                if(event.flags&EV_EOF){
                    EV_SET(&_data->kevset, _data->socket_fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
                    ff_close(_data->socket_fd);
                }
                else{
                    for(int i=0;i<event.data;++i){
                        if(_data->waiting_tasks.empty()){
                            std::cout<<"_data->waiting_tasks.empty()"<<std::endl;
                            _data->waiting_connections.emplace_back
                                    (tcp_session(kqfd,ff_accept(_data->socket_fd, nullptr, nullptr)));
                        }
                        else{
                            std::cout<<"not _data->waiting_tasks.empty()"<<std::endl;
                            auto&& t=_data->waiting_tasks.front();
                            t(std::forward<tcp_session>(tcp_session(kqfd,ff_accept(_data->socket_fd, nullptr, nullptr))));
                            _data->waiting_tasks.pop_front();
                        }

                    }
                }
            };
            ff_bind(_data->socket_fd, (struct linux_sockaddr *)&_data->addr, sizeof(_data->addr));
            ff_listen(_data->socket_fd, max_size);
            EV_SET(&_data->kevset, _data->socket_fd, EVFILT_READ, EV_ADD, 0, max_size, &_data->cb);
            ff_kevent(kqfd, &_data->kevset, 1, nullptr, 0, nullptr);
        }
    };

    auto
    start_tcp_listen(int kqfd, uint16_t port){
        return std::make_shared<tcp_listener>(kqfd,port);
    }
    auto
    wait_connect(std::shared_ptr<tcp_listener> l){
        return engine::reactor::flow_builder<tcp_session>::at_schedule(
                [l](std::shared_ptr<er::flow_implent<tcp_session>> f){
                    l->schedule([f](FLOW_ARG(tcp_session)&& v){
                        f->trigge(std::forward<FLOW_ARG(tcp_session)>(v));
                    });
                },
                er::_sp_immediate_runner_
        );
    }
}

#endif //PROJECT_LISTENER_HH
