//
// Created by root on 2020/5/4.
//

#ifndef PUMP_RESP_COMMAND_HH
#define PUMP_RESP_COMMAND_HH

#include <string>
#include <redis_agent/resped/resp_reply_decoder.hh>

namespace resp{
    class resp_command{
        
    };

    size_t get_key(const char* data, const decode::decode_result& cmd){
        return 1;
    }
}
#endif //PUMP_RESP_COMMAND_HH
