//
// Created by null on 20-2-1.
//
#include <string>
#include <iostream>
#include <tuple>
#include <cstring>
#include <reactor/flow.hh>
#include <dpdk/net/tcp_listener.hh>
#include <dpdk/engine.hh>
#include <common/unique_func.hh>
#include "mqtt_session.hh"
#include "mqtt_proc.hh"
#include "mqtt_cache.hh"
#include <reactor/keep_doing.hh>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "InfiniteRecursion"

std::shared_ptr<pump::dpdk::glb_context> this_context;

void
wait_connect_proc(std::shared_ptr<pump::dpdk::net::tcp_listener> l){
    pump::dpdk::net::wait_connect(l)
            .to_schedule(pump::dpdk::global::_sp_global_task_center_)
            .then([l](FLOW_ARG(pump::dpdk::net::tcp_session)&& v){
                ____forward_flow_monostate_exception(v);
                std::cout<<"new connection"<<std::endl;
                auto&& s=std::get<pump::dpdk::net::tcp_session>(v);
                mqtt::mqtt_proc<pump::dpdk::net::tcp_session>::mqtt_session_proc(std::forward<pump::dpdk::net::tcp_session>(s));
                wait_connect_proc(l);
            })
            .submit();
}
void
wait_channel_proc(){
    pump::dpdk::channel::recv_channel<pump::dpdk::channel::channel_msg>.wait_msg()
            .to_schedule(pump::dpdk::global::_sp_global_task_center_)
            .then([](FLOW_ARG(pump::dpdk::channel::channel_msg*)&& v){
                ____forward_flow_monostate_exception(v);
                pump::dpdk::channel::channel_msg* msg=std::get<pump::dpdk::channel::channel_msg*>(v);
                std::cout<<msg->data<<std::endl;
            })
            .then([](FLOW_ARG()&& v){
                wait_channel_proc();
            })
            .submit();
}
#pragma clang diagnostic pop

int main(int argc,char* argv[]){
    pump::dpdk::wait_engine_initialled(argc,argv)
            .then([](FLOW_ARG(std::shared_ptr<pump::dpdk::glb_context>)&& a){
                ____forward_flow_monostate_exception(a);

                std::cout<<"inited"<<std::endl;
                std::shared_ptr<pump::dpdk::glb_context> _c=std::get<std::shared_ptr<pump::dpdk::glb_context>>(a);
                this_context=_c;
                wait_channel_proc();
                wait_connect_proc(pump::dpdk::net::start_tcp_listen(_c->kqfd, 9022));
                return _c;
            })
            .then([](FLOW_ARG(std::shared_ptr<pump::dpdk::glb_context>)&& a){
                switch(a.index()){
                    case 0:
                        throw std::invalid_argument("std::monostate");
                    case 1:
                        std::rethrow_exception(std::get<std::exception_ptr>(a));
                    default:
                        try {
                            pump::dpdk::engine_run(
                                    std::get<std::shared_ptr<pump::dpdk::glb_context>>(a),
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

