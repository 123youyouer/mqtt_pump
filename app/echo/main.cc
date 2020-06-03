//
// Created by null on 20-2-1.
//

#include <iostream>
#include <cstring>
#include <string>
#include <variant>
#include <reactor/flow.hh>
#include <reactor/schedule.hh>
#include <dpdk/net/tcp_listener.hh>
#include <dpdk/net/tcp_connector.hh>
#include <dpdk/net/tcp_session.hh>
#include <dpdk/engine.hh>
#include <common/unique_func.hh>
#include <epoll/pump.hh>
#include "ee.hh"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
#pragma ide diagnostic ignored "InfiniteRecursion"
void
echo_proc(pump::dpdk::net::tcp_session&& session,int timeout){
    session.wait_packet(timeout)
            .to_schedule(pump::dpdk::global::_sp_global_task_center_)
            .then([s=std::forward<pump::dpdk::net::tcp_session>(session),timeout](FLOW_ARG(std::variant<int,pump::common::ringbuffer*>)&& v)mutable{
                ____forward_flow_monostate_exception(v);
                std::variant<int,pump::common::ringbuffer*> d=std::get<2>(v);
                switch(d.index()){
                    case 0:
                    {
                        std::cout<<"time out"<<std::endl;
                        char* sz=new char[10];
                        sprintf(sz,"time out");
                        return s.send_packet(sz,10);
                    }
                    default:
                    {
                        pump::common::ringbuffer* data=std::get<pump::common::ringbuffer*>(d);
                        size_t l=data->size();
                        char* sz=new char[l];
                        memcpy(sz,data->read_head(),l);
                        data->consume(l);
                        return s.send_packet(sz,l);
                    }
                }
            })
            .then([s=std::forward<pump::dpdk::net::tcp_session>(session),timeout](FLOW_ARG(std::tuple<const char*,size_t>)&& v)mutable{
                switch (v.index()){
                    case 0:
                        std::cout<<"unknown exception."<<std::endl;
                        s._data->close();
                        return;
                    case 1:
                        try{
                            s._data->close();
                            std::rethrow_exception(std::get<std::exception_ptr>(v));
                        }
                        catch (std::exception& e){
                            std::cout<<e.what()<<std::endl;
                        }
                        catch (...){
                            std::cout<<"unknown exception."<<std::endl;
                        }
                        return;
                    default:
                        auto [c,l]=std::get<std::tuple<const char*,size_t>>(v);
                        delete[] c;
                        echo_proc(std::forward<pump::dpdk::net::tcp_session>(s),timeout);
                }
            })
            .submit();
}

void
wait_connect_proc(std::shared_ptr<pump::dpdk::net::tcp_listener> l){
    pump::dpdk::net::wait_connect(l)
            .to_schedule(pump::dpdk::global::_sp_global_task_center_)
            .then([l](FLOW_ARG(pump::dpdk::net::tcp_session)&& v){
                ____forward_flow_monostate_exception(v);
                std::cout<<"new connection"<<std::endl;
                echo_proc(std::forward<pump::dpdk::net::tcp_session>(std::get<pump::dpdk::net::tcp_session>(v)),20000);
            })
            .then([l](FLOW_ARG()&& v){
                switch (v.index()){
                    case 0:
                        wait_connect_proc(l);
                        return;
                    default:
                        try{
                            std::rethrow_exception(std::get<std::exception_ptr>(v));
                        }
                        catch (std::exception& e){
                            std::cout<<e.what()<<std::endl;
                        }
                        catch (...){
                            std::cout<<"unknown exception."<<std::endl;
                        }
                }
            })
            .submit();
}
int
main(int argc, char * argv[]){
    std::cout<<__cplusplus<<std::endl;
    pump::dpdk::wait_engine_initialled(argc,argv)
            .then([](FLOW_ARG(std::shared_ptr<pump::dpdk::glb_context>)&& a){
                ____forward_flow_monostate_exception(a);
                std::cout<<"inited"<<std::endl;
                std::shared_ptr<pump::dpdk::glb_context> _c=std::get<std::shared_ptr<pump::dpdk::glb_context>>(a);
                return _c;
            })
            .then([](FLOW_ARG(std::shared_ptr<pump::dpdk::glb_context>)&& a){
                ____forward_flow_monostate_exception(a);

                auto _c=std::get<std::shared_ptr<pump::dpdk::glb_context>>(a);
                //auto _c=std::get<std::shared_ptr<dpdk::glb_context>>(a);

                pump::dpdk::net::connect(_c->kqfd,"10.0.0.107",9022,10000)
                        .then([](FLOW_ARG(std::variant<int,pump::dpdk::net::tcp_session>)&& a){
                            ____forward_flow_monostate_exception(a);
                            auto&& rtn=std::get<std::variant<int,pump::dpdk::net::tcp_session>>(a);
                            switch (rtn.index()){
                                case 0:
                                    std::cout<<"timeout"<<std::endl;
                                    throw std::logic_error("timeout");
                                default:
                                    auto&& session=std::get<pump::dpdk::net::tcp_session>(rtn);
                                    char* sz=new char[10];
                                    sprintf(sz,"time out");
                                    return session.send_packet(sz,10);
                            }
                        })
                        .submit();
                wait_connect_proc(pump::dpdk::net::start_tcp_listen(_c->kqfd, 9022));
                pump::dpdk::engine_run(
                        _c,
                        [](){}
                );
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
}
#pragma clang diagnostic pop