//
// Created by root on 2020/4/8.
//

#ifndef PUMP_REDIS_SESSION_HH
#define PUMP_REDIS_SESSION_HH

#include <engine/net/tcp_session.hh>
#include <engine/net/tcp_connector.hh>
#include <engine/reactor/schedule.hh>
#include <redis_agent/resp/all.hpp>
namespace redis_agent::redis{
    struct command_res_handler{
        common::ncpy_func<bool()> available;
        common::ncpy_func<void(FLOW_ARG(resp::result)&&)> handle;
    };
    class redis_session : boost::noncopyable{
        struct _inner_{
            engine::net::tcp_session tcp;
            std::queue<command_res_handler> waiting_handlers;
            explicit
            _inner_(engine::net::tcp_session&& tcp_session):tcp(std::forward<engine::net::tcp_session>(tcp_session)){
            }
        };
        std::shared_ptr<_inner_> inner_data;
        void
        check_and_update_command_result(common::ringbuffer* buf){
            resp::decoder dec;
            auto res=dec.decode((const char*)buf->read_head(),buf->size());
            switch (res.type()){
                case resp::completed:
                    buf->consume(res.size());
                    if(!inner_data->waiting_handlers.empty())
                        if(inner_data->waiting_handlers.front().available())
                            inner_data->waiting_handlers.front().handle(FLOW_ARG(resp::result)(std::forward<resp::result>(res)));
                    if(!inner_data->waiting_handlers.empty())
                        inner_data->waiting_handlers.pop();
                    break;
                case resp::incompleted:
                    return;
                case resp::error:
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
    };
    auto
    send_command(redis_session&& session, const char* cmd,size_t len){
        return session.inner_data->tcp.send_packet(cmd,len)
                .then([session=std::forward<redis_session>(session)](FLOW_ARG(std::tuple<const char*,size_t>)&& v)mutable{
                    ____forward_flow_monostate_exception(v);
                    auto [c,l]=std::get<std::tuple<const char*,size_t>>(v);
                    delete[] c;
                    return engine::reactor::flow_builder<resp::result>::at_schedule(
                            [session=std::forward<redis_session>(session)](std::shared_ptr<engine::reactor::flow_implent<resp::result>> sp_flow)mutable{
                                session.inner_data->waiting_handlers.emplace(command_res_handler{
                                        [sp_flow](){ return !sp_flow->called();},
                                        [sp_flow](FLOW_ARG(resp::result)&& v){
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
                                        }
                                });
                            },
                            engine::reactor::_sp_immediate_runner_
                    );
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
                            session.check_and_update_command_result(std::get<1>(d));
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
