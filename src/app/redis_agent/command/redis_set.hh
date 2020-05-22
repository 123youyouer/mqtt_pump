//
// Created by root on 2020/4/8.
//

#ifndef PUMP_REDIS_SET_HH
#define PUMP_REDIS_SET_HH

#include <cstddef>
#include <redis_agent/command/redis_command_name.hh>
#include <memory>
#include <redis_agent/command/redis_command.hh>

namespace redis_agent::command{

    struct redis_cmd{
        redis_cmd_name name;
        size_t len;
        char* buf;
        resp::decode_result segment_flag;
        virtual void init()=0;
        std::string_view get_segment(int i){
            if(i<segment_flag.children_len)
                return std::string_view(this->buf+this->segment_flag.children[i]->bgn,this->segment_flag.children[i]->end+1);
            else
                return std::string_view(nullptr,0);
        }
    };

    struct redis_cmd_append : public redis_cmd{
        std::string_view key;
        std::string_view value;
        void init()final{
            key=get_segment(1);
            value=get_segment(2);
        }
    };
    struct redis_cmd_decr : public redis_cmd{
        std::string_view key;
        void init()final{
            key=get_segment(1);
        }
    };
    struct redis_cmd_decrby : public redis_cmd{
        std::string_view key;
        std::string_view value;
        void init()final{
            key=get_segment(1);
            value=get_segment(2);
        }
    };
    struct redis_cmd_get : public redis_cmd{
        std::string_view key;
        void init()final{
            key=get_segment(1);
        }
    };
    struct redis_cmd_getbit : public redis_cmd{
        std::string_view key;
        std::string_view offset;
        void init()final{
            key=get_segment(1);
            offset=get_segment(2);
        }
    };
    struct redis_cmd_getrange : public redis_cmd{
        std::string_view key;
        std::string_view start;
        std::string_view end;
        void init()final{
            key=get_segment(1);
            start=get_segment(2);
            end=get_segment(3);
        }
    };
    struct redis_cmd_getset : public redis_cmd{
        std::string_view key;
        std::string_view value;
        void init()final{
            key=get_segment(1);
            value=get_segment(2);
        }
    };
    struct redis_cmd_incr : public redis_cmd{
        std::string_view key;
        void init()final{
            key=get_segment(1);
        }
    };
    struct redis_cmd_incrby : public redis_cmd{
        std::string_view key;
        std::string_view value;
        void init()final{
            key=get_segment(1);
            value=get_segment(2);
        }
    };
    struct redis_cmd_incrbyfloat : public redis_cmd{
        std::string_view key;
        std::string_view value;
        void init()final{
            key=get_segment(1);
            value=get_segment(2);
        }
    };
    struct redis_cmd_set : public redis_cmd{
        std::string_view key;
        std::string_view value;
        void init()final{
            key=get_segment(1);
            value=get_segment(2);
        }
    };
    struct redis_cmd_psetex : public redis_cmd{
        std::string_view key;
        std::string_view ms;
        std::string_view value;
        void init()final{
            key=get_segment(1);
            ms=get_segment(2);
            value=get_segment(3);
        }
    };
    struct redis_cmd_setbit : public redis_cmd{
        std::string_view key;
        std::string_view offset;
        std::string_view value;
        void init()final{
            key=get_segment(1);
            offset=get_segment(2);
            value=get_segment(3);
        }
    };
    struct redis_cmd_setex : public redis_cmd{
        std::string_view key;
        std::string_view sec;
        std::string_view value;
        void init()final{
            key=get_segment(1);
            sec=get_segment(2);
            value=get_segment(3);
        }
    };
    struct redis_cmd_setnx : public redis_cmd{
        std::string_view key;
        std::string_view value;
        void init()final{
            key=get_segment(1);
            value=get_segment(2);
        }
    };
    struct redis_cmd_setrange : public redis_cmd{
        std::string_view key;
        std::string_view offset;
        std::string_view value;
        void init()final{
            key=get_segment(1);
            offset=get_segment(2);
            value=get_segment(3);
        }
    };
    struct redis_cmd_strlen : public redis_cmd{
        std::string_view key;
        void init()final{
            key=get_segment(1);
        }
    };
}
#endif //PUMP_REDIS_SET_HH
