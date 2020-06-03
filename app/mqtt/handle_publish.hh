//
// Created by root on 2020/3/9.
//

#ifndef PUMP_HANDLE_PUBLISH_HH
#define PUMP_HANDLE_PUBLISH_HH

#include "mqtt_cache.hh"
#include "mqtt_packet.hh"
namespace mqtt{
    template <typename _SESSION_>
    auto
    handle_publish(_SESSION_&& session,const mqtt_pkt_publish& pkt){
        return pump::reactor::make_imme_flow();
    }
}
#endif //PUMP_HANDLE_PUBLISH_HH
