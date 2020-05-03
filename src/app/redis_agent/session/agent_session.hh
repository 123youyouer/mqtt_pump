//
// Created by root on 2020/4/30.
//

#ifndef PUMP_AGENT_SESSION_HH
#define PUMP_AGENT_SESSION_HH

#include <boost/noncopyable.hpp>
#include <engine/net/tcp_session.hh>
#include <redis_agent/resp/all.hpp>
#include <engine/net/tcp_listener.hh>

namespace redis_agent::session{
    class agent_session : boost::noncopyable{
        using handle_type=common::ncpy_func<void(FLOW_ARG(resp::result)&&)>;
        std::queue<resp::result> waiting_commands;
        std::queue<handle_type> waiting_handlers;
        engine::net::tcp_session&& session;
        void
        check_and_update_command_request(common::ringbuffer* buf){
            resp::decoder dec;
            auto cmd=dec.decode((const char*)buf->read_head(),buf->size());
            switch (cmd.type()){
                case resp::completed:
                    buf->consume(cmd.size());
                    if(waiting_handlers.empty()){
                        waiting_commands.emplace(std::forward<resp::result>(cmd));
                    }
                    else{
                        waiting_handlers.front()(FLOW_ARG(resp::result)(std::forward<resp::result>(cmd)));
                        waiting_handlers.pop();
                    }
                    break;
                case resp::incompleted:
                    return;
                case resp::error:
                    throw std::logic_error("redis client session decode error");
            }
        }
        void
        recv_proc(){
            session.wait_packet()
                    .then([this](FLOW_ARG(std::variant<int,common::ringbuffer*>)&& v){
                        ____forward_flow_monostate_exception(v);
                        std::variant<int,common::ringbuffer*> d=std::get<2>(v);
                        switch(d.index()){
                            case 0:
                                throw std::logic_error("timeout");
                            default:
                                check_and_update_command_request(std::get<1>(d));
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
    public:
        explicit
        agent_session(engine::net::tcp_session&& ts):session(std::forward<engine::net::tcp_session>(ts)){}
        void
        start(){
            recv_proc();
        }
        auto
        wait_command(){
            if(!waiting_commands.empty()){
                auto res=engine::reactor::make_imme_flow(std::forward<resp::result>(waiting_commands.front()));
                waiting_commands.pop();
                return res;
            }
            else{
                return engine::reactor::flow_builder<resp::result>::at_schedule(
                        [this](std::shared_ptr<engine::reactor::flow_implent<resp::result>> sp_flow){
                            waiting_handlers.emplace([sp_flow](FLOW_ARG(resp::result)&& v){
                                switch (v.index()){
                                    case 0:
                                        sp_flow->trigge(FLOW_ARG(resp::result)(std::get<0>(v)));
                                        return;
                                    case 1:
                                        sp_flow->trigge(FLOW_ARG(resp::result)(std::get<1>(v)));
                                        return;
                                    default:
                                        sp_flow->trigge(FLOW_ARG(resp::result)(std::get<2>(v)));
                                        return;
                                }
                            });
                        },
                        engine::reactor::_sp_immediate_runner_
                );
            }
        }
    };


    auto wait_connection(std::shared_ptr<engine::net::tcp_listener> listener){
        return engine::net::wait_connect(listener)
                .to_schedule(engine::reactor::_sp_global_task_center_)
                .then([listener](FLOW_ARG(engine::net::tcp_session)&& v){
                    ____forward_flow_monostate_exception(v);
                    std::cout<<"new connection"<<std::endl;
                    return agent_session(std::forward<engine::net::tcp_session>(std::get<engine::net::tcp_session>(v)));
                });
    }
}
#endif //PUMP_AGENT_SESSION_HH
