//
// Created by root on 2020/4/8.
//

#ifndef PUMP_REDIS_SESSION_HH
#define PUMP_REDIS_SESSION_HH

#include <engine/net/tcp_session.hh>
#include <engine/net/tcp_connector.hh>
#include <reactor/schedule.hh>
#include <redis_agent/resp/all.hpp>
#include <redis_agent/command/redis_command.hh>

namespace redis_agent::redis{
    struct command_res_handler{
        common::ncpy_func<bool()> available;
        common::ncpy_func<void(FLOW_ARG(std::unique_ptr<command::redis_reply>)&&)> handle;
    };
    class redis_session : boost::noncopyable{
        struct _inner_{
            engine::net::tcp_session tcp;
            std::queue<std::unique_ptr<command::redis_reply>> waiting_replies;
            std::queue<command_res_handler> waiting_handlers;
            explicit
            _inner_(engine::net::tcp_session&& tcp_session):tcp(std::forward<engine::net::tcp_session>(tcp_session)){}
        };
        std::shared_ptr<_inner_> inner_data;
        void
        decode_and_update_reply(common::ringbuffer* buf){
            auto rpl=std::make_unique<command::redis_reply>();
            size_t offset=0;
            switch (resp::decode(rpl->result,(const char*)buf->read_head(),offset,buf->size())){
                case resp::decode_state::st_complete:
                    rpl->len=offset+1;
                    rpl->buf=new char[rpl->len];
                    std::memcpy(rpl->buf,(const char*)buf->read_head(),rpl->len);
                    buf->consume(offset+1);
                    if(inner_data->waiting_handlers.empty()){
                        inner_data->waiting_replies.emplace(rpl);
                    }
                    else{
                        inner_data->waiting_handlers.front().handle
                                (FLOW_ARG(std::unique_ptr<command::redis_reply>)(std::forward<std::unique_ptr<command::redis_reply>>(rpl)));
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
        redis_session(engine::net::tcp_session&& tcp){
            inner_data=std::make_shared<_inner_>(std::forward<engine::net::tcp_session>(tcp));
        }
        redis_session(redis_session&& o)noexcept{
            inner_data=o.inner_data;
        }
        friend void read_proc(redis_session&& session);
        friend auto send_command(redis_session&& session, const char* cmd,size_t len);
        friend auto wait_reply(redis_session&& session);
    };
    auto
    wait_reply(redis_session&& session){
        if(!session.inner_data->waiting_replies.empty()){
            auto res=reactor::make_imme_flow(std::move(session.inner_data->waiting_replies.front()));
            session.inner_data->waiting_replies.pop();
            return res;
        }
        else{
            return reactor::flow_builder<std::unique_ptr<command::redis_reply>>::at_schedule(
                    [session=std::forward<redis_session>(session)](std::shared_ptr<reactor::flow_implent<std::unique_ptr<command::redis_reply>>> sp_flow)mutable{
                        session.inner_data->waiting_handlers.emplace(command_res_handler{
                                [sp_flow](){ return !sp_flow->called();},
                                [sp_flow](FLOW_ARG(std::unique_ptr<command::redis_reply>)&& v){
                                    sp_flow->trigge(std::forward<FLOW_ARG(std::unique_ptr<command::redis_reply>)>(v));
                                }
                        });
                    },
                    reactor::_sp_immediate_runner_
            );
        }
    }
    auto
    send_command(redis_session&& session, const char* cmd,size_t len){
        return session.inner_data->tcp.send_packet(cmd,len)
                .then([session=std::forward<redis_session>(session)](FLOW_ARG(std::tuple<const char*,size_t>)&& v)mutable{
                    ____forward_flow_monostate_exception(v);
                    return wait_reply(std::forward<redis_session>(session));
                });
    }
    void
    read_proc(redis_session&& session){
        session.inner_data->tcp.wait_packet()
                .then([session=std::forward<redis_session>(session)](FLOW_ARG(std::variant<int,common::ringbuffer*>)&& v)mutable{
                    ____forward_flow_monostate_exception(v);
                    std::variant<int,common::ringbuffer*> d=std::get<2>(v);
                    switch(d.index()){
                        case 0:
                            throw std::logic_error("timeout");
                        default:
                            session.decode_and_update_reply(std::get<1>(d));
                            return;
                    }
                })
                .then([session=std::forward<redis_session>(session)](FLOW_ARG()&& v)mutable{
                    switch (v.index()){
                        case 0:
                            read_proc(std::forward<redis_session>(session));
                            break;
                        case 1:
                            return;
                    }
                })
                .submit();
    }
    auto
    connect_redis(int kqfd, const char* ip,int port,int timeout){
        return engine::net::connect(kqfd,ip,port,timeout)
                .then([](FLOW_ARG(std::variant<int,engine::net::tcp_session>)&& a){
                    ____forward_flow_monostate_exception(a);
                    auto&& rtn=std::get<std::variant<int,engine::net::tcp_session>>(a);
                    switch (rtn.index()){
                        case 0:
                            std::cout<<"timeout"<<std::endl;
                            throw std::logic_error("timeout");
                        default:
                            return redis_session(std::move(std::get<engine::net::tcp_session>(rtn)));
                    }
                });
    }

    void test(){
        connect_redis(10,"127.0.0.1",9000,1000)
                .then([](FLOW_ARG(redis_session)&& v){
                    ____forward_flow_monostate_exception(v);
                    auto&& session=std::get<redis_session>(v);
                    read_proc(std::forward<redis_session>(session));
                    return send_command(std::forward<redis_session>(session),"1",1);
                })
                .then([](FLOW_ARG(resp::result)&& v){
                    ____forward_flow_monostate_exception(v);
                })
                .submit();
    }
}
#endif //PUMP_REDIS_SESSION_HH
