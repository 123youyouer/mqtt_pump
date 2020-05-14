//
// Created by root on 2020/5/14.
//

#ifndef PUMP_REDIS_COMMAND_HH
#define PUMP_REDIS_COMMAND_HH

#include <string>
#include <redis_agent/resped/resp_decoder.hh>

namespace redis_agent::command{
    struct redis_command_flag{
        const static size_t write   =1;
        const static size_t has_k   =1<<1;
    };
    struct redis_command{
        size_t  len;
        char* buf;
        resp::decode_result result;
        size_t flags;
        __always_inline bool
        is_write_command(){
            return flags&redis_command_flag::write;
        }
        __always_inline bool
        has_key(){
            return flags&redis_command_flag::has_k;
        }
        __always_inline size_t
        get_key_hash(){
            return 1;
        }
        __always_inline const std::string&
        get_key(){
            return "";
        }
        redis_command():len(0),buf(nullptr),flags(0){}
    };
    struct redis_reply{
        size_t  len;
        char* buf;
        resp::decode_result result;
        redis_reply():len(0),buf(nullptr){}
    };
    struct command_context{
        std::unique_ptr<redis_command> cmd;
        std::unique_ptr<redis_reply> rpl;
    };
}
#endif //PUMP_REDIS_COMMAND_HH
