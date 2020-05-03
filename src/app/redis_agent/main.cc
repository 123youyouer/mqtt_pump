//
// Created by root on 2020/4/8.
//

#include <redis_agent/session/redis_session.hh>
#include <redis_agent/session/agent_session.hh>

void redis_command_agent_proc(redis_agent::session::agent_session&& session){
    session.wait_command()
            .then([](FLOW_ARG(resp::result)&& v){
                ____forward_flow_monostate_exception(v);
                //find key
                //find redis_sesson(cpu) by key in hash-ring
                //send command and wait response
                //back to this cpu and send result
            })
            .then([s=std::forward<redis_agent::session::agent_session>(session)](FLOW_ARG()&& v)mutable{
                switch (v.index()){
                    case 0:
                        redis_command_agent_proc(std::forward<redis_agent::session::agent_session>(s));
                        return;
                    default:
                        return;
                }
            })
            .submit();
}

void listen_proc(std::shared_ptr<engine::net::tcp_listener> l){
    redis_agent::session::wait_connection(l)
            .then([](FLOW_ARG(redis_agent::session::agent_session)&& v){
                ____forward_flow_monostate_exception(v);
                redis_command_agent_proc(std::forward<redis_agent::session::agent_session>(std::get<redis_agent::session::agent_session>(v)));
            })
            .then([l](FLOW_ARG()&& v){
                switch (v.index()){
                    case 0:
                        listen_proc(l);
                        return;
                    default:
                        return;
                }
            })
            .submit();
}

int main(int argc, char * argv[]){



    redis_agent::session::wait_connection(engine::net::start_tcp_listen(100, 9022))
            .then([](FLOW_ARG(redis_agent::session::agent_session)&& v){
                ____forward_flow_monostate_exception(v);
                redis_command_agent_proc(std::forward<redis_agent::session::agent_session>(std::get<redis_agent::session::agent_session>(v)));
            })
            .then([](FLOW_ARG()&& v){

            })
            .submit();
    return 0;
}