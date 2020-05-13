//
// Created by root on 2020/4/30.
//

#ifndef PUMP_AGENT_SESSION_HH
#define PUMP_AGENT_SESSION_HH

#include <boost/noncopyable.hpp>
#include <engine/net/tcp_session.hh>
#include <redis_agent/resped/resp_reply_decoder.hh>
#include <engine/net/tcp_listener.hh>

namespace redis_agent::agent{
    struct decode_struct:boost::noncopyable{
        size_t  len;
        char* buf;
        resp::decode::decode_result res;
        decode_struct():len(0),buf(nullptr){};
    };
    class agent_session : boost::noncopyable{
    public:
        using decode_type=std::unique_ptr<decode_struct>;
        using handle_type=common::ncpy_func<void(FLOW_ARG(decode_type)&&)>;
    private:
        struct _inner_{
            std::queue<decode_type> waiting_commands;
            std::queue<handle_type> waiting_handlers;
            engine::net::tcp_session tcp;
            explicit
            _inner_(engine::net::tcp_session&& t):tcp(std::forward<engine::net::tcp_session>(t)){}
        };
        std::shared_ptr<_inner_> inner_data;
        void
        check_and_update_command_request(common::ringbuffer* buf){
            decode_type cmd=std::make_unique<decode_struct>();
            size_t offset=0;
            switch (resp::decode::decode(cmd->res,(const char*)buf->read_head(),offset,buf->size())){
                case resp::decode::decode_state::st_complete:
                    cmd->len=offset+1;
                    cmd->buf=new char[cmd->len];
                    std::memcpy(cmd->buf,(const char*)buf->read_head(),cmd->len);
                    buf->consume(offset+1);
                    if(inner_data->waiting_handlers.empty()){
                        inner_data->waiting_commands.emplace(std::forward<decode_type>(cmd));
                    }
                    else{
                        inner_data->waiting_handlers.front()(FLOW_ARG(decode_type)(std::forward<decode_type>(cmd)));
                        inner_data->waiting_handlers.pop();
                    }
                    return;
                case resp::decode::decode_state::st_incomplete:
                    return;
                case resp::decode::decode_state::st_decode_error:
                    throw std::logic_error("redis client session decode error");
            }
        }
    public:
        explicit
        agent_session(engine::net::tcp_session&& ts):inner_data(std::make_shared<_inner_>(std::forward<engine::net::tcp_session>(ts))){
        }
        agent_session(agent_session&& o)noexcept{
            inner_data=o.inner_data;
        };
        friend auto send_command(agent_session&& session, const char* cmd,size_t len);
        friend auto wait_command(agent_session&& session);
        friend void read_proc(agent_session&& session);
    };

    auto
    send_command(agent_session&& session, const char* cmd,size_t len){
        return session.inner_data->tcp.send_packet(cmd,len);
    }

    auto
    wait_command(agent_session&& session){
        if(!session.inner_data->waiting_commands.empty()){
            auto res=engine::reactor::make_imme_flow(std::move(session.inner_data->waiting_commands.front()));
            session.inner_data->waiting_commands.pop();
            return res;
        }
        else{
            return engine::reactor::flow_builder<agent_session::decode_type>::at_schedule(
                    [session=std::forward<agent_session>(session)](std::shared_ptr<engine::reactor::flow_implent<agent_session::decode_type>> sp_flow){
                        session.inner_data->waiting_handlers.emplace([sp_flow](FLOW_ARG(agent_session::decode_type)&& v){
                            //sp_flow->trigge(std::forward<FLOW_ARG(decode_type)>(v));
                        });
                    },
                    engine::reactor::_sp_immediate_runner_
            );
        }
    }

    void
    read_proc(agent_session&& session){
        session.inner_data->tcp.wait_packet()
                .then([session=std::forward<agent_session>(session)](FLOW_ARG(std::variant<int,common::ringbuffer*>)&& v)mutable{
                    ____forward_flow_monostate_exception(v);
                    std::variant<int,common::ringbuffer*> d=std::get<2>(v);
                    switch(d.index()){
                        case 0:
                            throw std::logic_error("timeout");
                        default:
                            session.check_and_update_command_request(std::get<1>(d));
                            return;
                    }
                })
                .then([session=std::forward<agent_session>(session)](FLOW_ARG()&& v)mutable{
                    switch (v.index()){
                        case 0:
                            read_proc(std::forward<agent_session>(session));
                            break;
                        case 1:
                            return;
                    }
                })
                .submit();
    }


    auto
    wait_connection(std::shared_ptr<engine::net::tcp_listener> listener){
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
