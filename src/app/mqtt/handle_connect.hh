//
// Created by root on 2020/3/3.
//

#ifndef PUMP_HANDLE_CONNECT_HH
#define PUMP_HANDLE_CONNECT_HH
namespace mqtt{
    struct mqtt_pkt_connect{
        u_int8_t    remaining;
        u_int8_t    protocol_name[6];
        u_int8_t    protocol_level;
        u_int8_t    connect_flags;
        u_int16_t   keep_alive;

        int from_buf(common::ringbuffer*){
            return 0;
        }
        size_t remaining_bytes(){
            return remaining;
        }
        int build_pkt(common::ringbuffer*){
            return 1;
        }
    };
    auto
    handle_connect(mqtt_pkt_connect& pkt){
    }
}
#endif //PUMP_HANDLE_CONNECT_HH
