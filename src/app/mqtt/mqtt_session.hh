//
// Created by root on 20-2-26.
//

#ifndef PROJECT_MQTT_SESSION_H
#define PROJECT_MQTT_SESSION_H

#include "mqtt_packet.hh"

namespace mqtt{
    template <typename _TCP_>
    struct mqtt_session{
        struct data{
            mqtt_pkt_connect session_info;
            _TCP_ inner_tcp;
            explicit
            data(_TCP_&& tcp):inner_tcp(std::forward<_TCP_>(tcp)){}
        };
        std::shared_ptr<data> _inner_data_;
        explicit
        mqtt_session(_TCP_&& impl):_inner_data_(std::make_shared<data>(std::forward<_TCP_>(impl))){}

        mqtt_session(const mqtt_session&& o)noexcept:_inner_data_(o._inner_data_){}


        void
        set_connect_info(const mqtt_pkt_connect& pkt){
            _inner_data_->session_info.data=pkt.data;
        }

        const char*
        get_id()const{
            return _inner_data_->session_info.data->client_id.c_str();
        }

        ALWAYS_INLINE auto
        send_packet(char* buf,size_t len){
            return _inner_data_->inner_tcp.send_packet(buf,len);
        }
    };



}



#endif //PROJECT_MQTT_SESSION_H
