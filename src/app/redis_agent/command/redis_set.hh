//
// Created by root on 2020/4/8.
//

#ifndef PUMP_REDIS_SET_HH
#define PUMP_REDIS_SET_HH

#include <cstddef>

namespace redis_agent::command{
    class set{
    public:
        static bool check_command_type(const char* cmd,size_t len){
            return len>4
                   && (cmd[0]=='s'||cmd[0]=='S')
                   && (cmd[1]=='e'||cmd[1]=='E')
                   && (cmd[2]=='t'||cmd[2]=='T')
                   && (cmd[2]==' ');
        }
    };

    void parse_command(const char* cmd,size_t len){
        if(set::check_command_type(cmd,len)){

        }
    }
}
#endif //PUMP_REDIS_SET_HH
