//
// Created by root on 2020/3/3.
//

#ifndef PUMP_HANDLE_PINGREQ_HH
#define PUMP_HANDLE_PINGREQ_HH

#include "mqtt_packet.hh"

namespace mqtt{
    struct mqtt_pkt_pingreq{
        int from_buf(common::ringbuffer*){
            return 0;
        }
        size_t remaining_bytes(){
            return 10;
        }
        int build_pkt(common::ringbuffer*){
            return 1;
        }
    };
    auto
    handle_pingreq(){}
}
#endif //PUMP_HANDLE_PINGREQ_HH
