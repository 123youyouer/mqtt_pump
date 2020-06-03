//
// Created by root on 2020/4/30.
//

#ifndef PUMP_AGENT_SESSION_HH
#define PUMP_AGENT_SESSION_HH

#include <boost/noncopyable.hpp>
#include <dpdk/net/tcp_session.hh>
#include <dpdk/global/global_task_center.hh>
#include <dpdk/net/tcp_listener.hh>
#include <redis_agent/resped/resp_decoder.hh>
#include <redis_agent/command/redis_command.hh>

namespace redis_agent::agent{
    class agent_session : boost::noncopyable{
    public:
        using decode_type=std::unique_ptr<command::redis_command>;
        using handle_type=pump::common::ncpy_func<void(FLOW_ARG(decode_type)&&)>;
    private:
        struct _inner_{
            std::queue<decode_type> waiting_commands;
            std::queue<handle_type> waiting_handlers;
            pump::dpdk::net::tcp_session tcp;
            explicit
            _inner_(pump::dpdk::net::tcp_session&& t):tcp(std::forward<pump::dpdk::net::tcp_session>(t)){}
        };
        std::shared_ptr<_inner_> inner_data;
        void
        decode_and_notify_command(pump::common::ringbuffer* buf){
            decode_type cmd=std::make_unique<command::redis_command>();
            size_t offset=0;
            switch (resp::decode(cmd->result,(const char*)buf->read_head(),offset,buf->size())){
                case resp::decode_state::st_complete:
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
                case resp::decode_state::st_incomplete:
                    return;
                case resp::decode_state::st_decode_error:
                    throw std::logic_error("redis client session decode error");
            }
        }
    public:
        explicit
        agent_session(pump::dpdk::net::tcp_session&& ts):inner_data(std::make_shared<_inner_>(std::forward<pump::dpdk::net::tcp_session>(ts))){
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
            auto res=pump::reactor::make_imme_flow(std::move(session.inner_data->waiting_commands.front()));
            session.inner_data->waiting_commands.pop();
            return res;
        }
        else{
            return pump::reactor::flow_builder<agent_session::decode_type>::at_schedule(
                    [session=std::forward<agent_session>(session)](std::shared_ptr<pump::reactor::flow_implent<agent_session::decode_type>> sp_flow){
                        session.inner_data->waiting_handlers.emplace([sp_flow](FLOW_ARG(agent_session::decode_type)&& v){
                            //sp_flow->trigge(std::forward<FLOW_ARG(decode_type)>(v));
                        });
                    },
                    pump::reactor::_sp_immediate_runner_
            );
        }
    }

    void
    read_proc(agent_session&& session){
        session.inner_data->tcp.wait_packet()
                .then([session=std::forward<agent_session>(session)](FLOW_ARG(std::variant<int,pump::common::ringbuffer*>)&& v)mutable{
                    ____forward_flow_monostate_exception(v);
                    std::variant<int,pump::common::ringbuffer*> d=std::get<2>(v);
                    switch(d.index()){
                        case 0:
                            throw std::logic_error("timeout");
                        default:
                            session.decode_and_notify_command(std::get<1>(d));
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
    wait_connection(std::shared_ptr<pump::dpdk::net::tcp_listener> listener){
        return pump::dpdk::net::wait_connect(listener)
                .to_schedule(pump::dpdk::global::_sp_global_task_center_)
                .then([listener](FLOW_ARG(pump::dpdk::net::tcp_session)&& v){
                    ____forward_flow_monostate_exception(v);
                    std::cout<<"new connection"<<std::endl;
                    return agent_session(std::forward<pump::dpdk::net::tcp_session>(std::get<pump::dpdk::net::tcp_session>(v)));
                });
    }
}
#endif //PUMP_AGENT_SESSION_HH
