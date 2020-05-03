//
// Created by root on 2020/4/8.
//

#ifndef PUMP_REDIS_SESSION_HH
#define PUMP_REDIS_SESSION_HH

#include <engine/net/tcp_session.hh>
#include <engine/net/tcp_connector.hh>
#include <engine/reactor/schedule.hh>
#include <redis_agent/resp/all.hpp>
namespace redis_agent::session{
    class redis_session : boost::noncopyable{
        struct command_res_handler{
            common::ncpy_func<bool()> available;
            common::ncpy_func<void(FLOW_ARG(resp::result)&&)> handle;
        };
        engine::net::tcp_session&& session;
        std::queue<command_res_handler> waiting_handlers;
        void check_and_update_command_result(common::ringbuffer* buf){
            resp::decoder dec;
            auto res=dec.decode((const char*)buf->read_head(),buf->size());
            switch (res.type()){
                case resp::completed:
                    buf->consume(res.size());
                    if(!waiting_handlers.empty())
                        if(waiting_handlers.front().available())
                            waiting_handlers.front().handle(FLOW_ARG(resp::result)(std::forward<resp::result>(res)));
                    if(!waiting_handlers.empty())
                        waiting_handlers.pop();
                    break;
                case resp::incompleted:
                    return;
                case resp::error:
                    throw std::logic_error("redis client session decode error");
            }
        }
        void recv_proc(){
            session.wait_packet()
                    .then([this](FLOW_ARG(std::variant<int,common::ringbuffer*>)&& v){
                        ____forward_flow_monostate_exception(v);
                        std::variant<int,common::ringbuffer*> d=std::get<2>(v);
                        switch(d.index()){
                            case 0:
                                throw std::logic_error("timeout");
                            default:
                                check_and_update_command_result(std::get<1>(d));
                                return;
                        }
                    })
                    .then([this](FLOW_ARG()&& v)mutable{
                        switch (v.index()){
                            case 0:
                                recv_proc();
                                break;
                            case 1:
                                return;
                        }
                    })
                    .submit();
        }
        auto
        wait_cmd_result(u_int64_t ms){
            return engine::reactor::flow_builder<std::variant<int,resp::result>>::at_schedule(
                    [this,ms](std::shared_ptr<engine::reactor::flow_implent<std::variant<int,resp::result>>> sp_flow){
                        waiting_handlers.emplace(command_res_handler{
                                [sp_flow](){ return !sp_flow->called();},
                                [sp_flow](FLOW_ARG(resp::result)&& v){
                                    switch (v.index()){
                                        case 0:
                                            sp_flow->trigge(FLOW_ARG(std::variant<int,resp::result>)(std::get<0>(v)));
                                            return;
                                        case 1:
                                            sp_flow->trigge(FLOW_ARG(std::variant<int,resp::result>)(std::get<1>(v)));
                                            return;
                                        default:
                                            sp_flow->trigge(FLOW_ARG(std::variant<int,resp::result>)(
                                                    std::variant<int,resp::result>(std::get<2>(v))));
                                            return;
                                    }
                                }
                        });
                        engine::timer::_sp_timer_set->add_timer(ms,[ms,sp_flow](){
                            sp_flow->trigge(FLOW_ARG(std::variant<int,resp::result>)(
                                    std::variant<int,resp::result>(ms)));
                        });
                    },
                    engine::reactor::_sp_global_task_center_
            );
        }
    public:
        auto
        start(){
            recv_proc();
        }
        auto
        send_command(const char* cmd,size_t len,u_int64_t timeout){
            return session.send_packet(cmd,len)
                    .then([this,timeout](FLOW_ARG(std::tuple<const char*,size_t>)&& v){
                        ____forward_flow_monostate_exception(v);
                        auto [c,l]=std::get<std::tuple<const char*,size_t>>(v);
                        delete[] c;
                        return wait_cmd_result(timeout);
                    });
        }
    };
    auto connect_redis(int kqfd, const char* ip,int port,int timeout){
        return engine::net::connect(kqfd,ip,port,timeout)
                .then([](FLOW_ARG(std::variant<int,engine::net::tcp_session>)&& a){
                    ____forward_flow_monostate_exception(a);
                    auto&& rtn=std::get<std::variant<int,engine::net::tcp_session>>(a);
                    switch (rtn.index()){
                        case 0:
                            std::cout<<"timeout"<<std::endl;
                            throw std::logic_error("timeout");
                        default:
                            auto&& session=std::get<engine::net::tcp_session>(rtn);
                            return;
                    }
                });
    }

    void test(){

    }
}
#endif //PUMP_REDIS_SESSION_HH
