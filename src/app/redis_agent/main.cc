//
// Created by root on 2020/4/8.
//

#include <redis_agent/redis/redis_session.hh>
#include <redis_agent/agent/agent_session.hh>
#include <gtest/gtest.h>
#include <redis_agent/resped/resp_command.hh>
#include <common/consistent_hashing.hh>
#include <redis_agent/redis/redis_nodes.hh>
namespace redis_agent{
    void redis_command_agent_proc(agent::agent_session&& session){
        agent::wait_command(std::forward<agent::agent_session>(session))
                .then([s=std::forward<agent::agent_session>(session)](FLOW_ARG(agent::agent_session::decode_type)&& v)mutable{
                    ____forward_flow_monostate_exception(v);
                    redis_command_agent_proc(std::forward<agent::agent_session>(s));
                    auto cmd(std::forward<agent::agent_session::decode_type>(std::get<agent::agent_session::decode_type>(v)));
                    auto node=redis::find_redis_node_by_key(resp::get_key(cmd->buf,cmd->res));
                    return redis::send_command(std::forward<redis::redis_session>(node->second.redis_data->session),cmd->buf,cmd->len);
                })
                .then([s=std::forward<agent::agent_session>(session)](FLOW_ARG(resp::result)&& v)mutable{
                    ____forward_flow_monostate_exception(v);
                    return agent::send_command(std::forward<agent::agent_session>(s),"",1);
                })
                .then([](FLOW_ARG(std::tuple<const char*,size_t>)&& v){
                    ____forward_flow_monostate_exception(v);
                })
                .then([s=std::forward<agent::agent_session>(session)](FLOW_ARG()&& v)mutable{
                    switch (v.index()){
                        case 0:
                            redis_command_agent_proc(std::forward<agent::agent_session>(s));
                            return;
                        default:
                            return;
                    }
                })
                .submit();
    }


    void listen_proc(std::shared_ptr<engine::net::tcp_listener> l){
        agent::wait_connection(l)
                .then([](FLOW_ARG(agent::agent_session)&& v){
                    ____forward_flow_monostate_exception(v);
                    auto&& session=std::get<agent::agent_session>(v);
                    agent::read_proc(std::forward<agent::agent_session>(session));
                    redis_command_agent_proc(std::forward<agent::agent_session>(session));
                })
                .then([l](FLOW_ARG()&& v){
                    switch (v.index()){
                        case 0:
                            return listen_proc(l);
                        default:
                            return;
                    }
                })
                .submit();
    }


    int main(int argc, char * argv[]){

        redis::redis_servers.emplace(redis::consistent_hash_node{std::shared_ptr<redis::redis_node>(nullptr),1});
        redis::redis_servers.emplace(redis::consistent_hash_node{std::shared_ptr<redis::redis_node>(nullptr),10});
        redis::redis_servers.emplace(redis::consistent_hash_node{std::shared_ptr<redis::redis_node>(nullptr),15});
        redis::redis_servers.emplace(redis::consistent_hash_node{std::shared_ptr<redis::redis_node>(nullptr),100});
        auto x=redis::redis_servers.find(12);
        std::cout<<x->second.redis_data->ip<<std::endl;
        return 0;
    }
}