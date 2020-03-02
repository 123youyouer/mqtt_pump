//
// Created by root on 20-2-26.
//

#ifndef PROJECT_MQTT_SESSION_H
#define PROJECT_MQTT_SESSION_H

#include <engine/net/tcp_session.hh>

namespace mqtt{
    enum class
    mqtt_command_type : std::uint8_t {
        reserved    = 0b00000000,
        connect     = 0b00010000, // 1
        connack     = 0b00100000, // 2
        publish     = 0b00110000, // 3
        puback      = 0b01000000, // 4
        pubrec      = 0b01010000, // 5
        pubrel      = 0b01100000, // 6
        pubcomp     = 0b01110000, // 7
        subscribe   = 0b10000000, // 8
        suback      = 0b10010000, // 9
        unsubscribe = 0b10100000, // 10
        unsuback    = 0b10110000, // 11
        pingreq     = 0b11000000, // 12
        pingresp    = 0b11010000, // 13
        disconnect  = 0b11100000, // 14
        auth        = 0b11110000, // 15
    };
    auto
    mqtt_wait_connected(engine::net::tcp_session&& session){

    }
}



#endif //PROJECT_MQTT_SESSION_H
