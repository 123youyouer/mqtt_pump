//
// Created by null on 20-2-1.
//
#include <string>
#include <iostream>
#include <tuple>
#include <cstring>
#include <engine/reactor/flow.hh>
#include <engine/net/tcp_listener.hh>
#include <engine/engine.hh>
#include <common/unique_func.hh>
#include "mqtt_session.hh"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "InfiniteRecursion"
void
echo_proc(engine::net::tcp_session&& session,int timeout){
    session.wait_packet(timeout)
            .to_schedule(engine::reactor::_sp_global_task_center_)
            .then([s=std::forward<engine::net::tcp_session>(session),timeout](FLOW_ARG(std::variant<int,common::ringbuffer*>)&& v)mutable{
                ____forward_flow_monostate_exception(v);
                std::variant<int,common::ringbuffer*> d=std::get<2>(v);
                switch(d.index()){
                    case 0:
                    {
                        std::cout<<"time out"<<std::endl;
                        char* sz=new char[10];
                        sprintf(sz,"time out");
                        s.send_packet(sz,10)
                                .to_schedule(engine::reactor::_sp_global_task_center_)
                                .then([s=std::forward<engine::net::tcp_session>(s),sz](FLOW_ARG(engine::net::send_proxy)&& v)mutable{
                                    delete[] sz;
                                    s._data->close();
                                })
                                .submit();
                        break;
                    }
                    case 1:
                    {
                        common::ringbuffer* data=std::get<common::ringbuffer*>(d);
                        size_t l=data->size();
                        char* sz=new char[l];
                        memcpy(sz,data->read_head(),l);
                        data->consume(l);
                        s.send_packet(sz,l)
                                .to_schedule(engine::reactor::_sp_global_task_center_)
                                .then([sz](FLOW_ARG(engine::net::send_proxy)&& v)mutable{
                                    delete[] sz;
                                })
                                .submit();
                        echo_proc(std::forward<engine::net::tcp_session>(s),timeout);
                        break;
                    }
                }

            })
            .submit();
}

void
wait_connect_proc(std::shared_ptr<engine::net::tcp_listener> l){
    engine::net::wait_connect(l)
            .to_schedule(engine::reactor::_sp_global_task_center_)
            .then([l](FLOW_ARG(engine::net::tcp_session)&& v){
                ____forward_flow_monostate_exception(v);
                std::cout<<"new connection"<<std::endl;
                auto&& s=std::get<engine::net::tcp_session>(v);
                mqtt::operation<engine::net::tcp_session>::mqtt_session_proc(std::forward<engine::net::tcp_session>(s));
                wait_connect_proc(l);
            })
            .submit();
}
template <typename F> engine::reactor::flow_builder<std::result_of_t<F(FLOW_ARG()&&)>>
keep_doing(F&& f){
    using _R_=std::result_of_t<F(FLOW_ARG()&&)>;
    return engine::reactor::make_task_flow()
            .then(std::forward<F>(f))
            .then([f=std::forward<F>(f)](FLOW_ARG(_R_)&& v){
                ____forward_flow_monostate_exception(v);
                auto r=std::get<_R_>(v);
                if(!r)
                    return engine::reactor::make_imme_flow()
                            .then([r=std::forward<_R_>(r)](FLOW_ARG()&& v){
                                return r;
                            })
                            .to_schedule(engine::reactor::_sp_global_task_center_);
                else
                    return keep_doing(f);
            });
}
#pragma clang diagnostic pop

int main(int argc,char* argv[]){
    engine::reactor::make_imme_flow()
            .then([](FLOW_ARG()&& v){
                ____forward_flow_monostate_exception(v);
                std::cout<<"11111"<<std::endl;
            })
            .then([](FLOW_ARG()&& v){
                ____forward_flow_monostate_exception(v);
                return engine::reactor::make_imme_flow()
                        .then([](FLOW_ARG()&& v){
                            ____forward_flow_monostate_exception(v);
                            std::cout<<"BBBBBB"<<std::endl;
                            return engine::reactor::make_imme_flow()
                                    .then([](FLOW_ARG()&& v){
                                        std::cout<<"CCCCCCCC"<<std::endl;
                                        return;
                                    });
                        });
            })
            .then([](FLOW_ARG()&& v){
                ____forward_flow_monostate_exception(v);
                std::cout<<"AAAA"<<std::endl;
            })
            .submit();

    int i=10;
    keep_doing([&i](FLOW_ARG()&& v){
        std::cout<<i<<std::endl;
        return --i;
    })
            .then([](FLOW_ARG(int)&& v){
                std::cout<<1234<<std::endl;
            })
            .submit();

    for(int i=0;i<1000;++i)

    engine::reactor::_sp_global_task_center_->run();

    engine::wait_engine_initialled(argc,argv)
            .then([](FLOW_ARG(std::shared_ptr<engine::glb_context>)&& a){
                ____forward_flow_monostate_exception(a);
                std::cout<<"inited"<<std::endl;
                std::shared_ptr<engine::glb_context> _c=std::get<std::shared_ptr<engine::glb_context>>(a);
                wait_connect_proc(engine::net::start_tcp_listen(_c->kqfd, 9022));
                return _c;
            })
            .then([](FLOW_ARG(std::shared_ptr<engine::glb_context>)&& a){
                switch(a.index()){
                    case 0:
                        throw std::invalid_argument("std::monostate");
                    case 1:
                        std::rethrow_exception(std::get<std::exception_ptr>(a));
                    default:
                        try {
                            engine::engine_run(
                                    std::get<std::shared_ptr<engine::glb_context>>(a),
                                    [](){}
                            );
                        }
                        catch (...){
                            throw std::current_exception();
                        }
                }
            })
            .then([](FLOW_ARG()&& a){
                switch(a.index()){
                    case 0:
                        std::cout<<"std::monostate"<<std::endl;
                    default:
                        try {
                            std::rethrow_exception(std::get<std::exception_ptr>(a));
                        }
                        catch (std::exception& e){
                            std::cout<<e.what()<<std::endl;
                        }
                        catch (...){
                            std::cout<<"unkonwn exception ... "<<std::endl;
                        }
                }
            })
            .submit();

    return 0;
}

