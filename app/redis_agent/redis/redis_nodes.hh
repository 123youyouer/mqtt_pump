//
// Created by root on 2020/5/13.
//

#ifndef PUMP_REDIS_NODES_HH
#define PUMP_REDIS_NODES_HH

#include <string>
#include <common/consistent_hashing.hh>
#include <redis_agent/redis/redis_session.hh>
namespace redis_agent::redis{
    struct redis_node{
        std::string ip;
        u_int16_t port;
        u_int16_t state;
        redis_session session;
    };

    struct consistent_hash_node{
        std::shared_ptr<redis_node> redis_data;
        size_t last_key;
        size_t get_key(){
            return last_key;
        }
    };

    pump::common::consistent_hash<size_t,consistent_hash_node> redis_servers;

    auto
    find_redis_node_by_key(const std::string& k){
        return redis_servers.find(10);
    }

    auto
    find_redis_node_by_key(const size_t& k){
        return redis_servers.find(k);
    }

}
#endif //PUMP_REDIS_NODES_HH
