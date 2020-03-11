//
// Created by root on 20-2-26.
//

#ifndef PROJECT_MQTT_SESSION_H
#define PROJECT_MQTT_SESSION_H

#include "mqtt_packet.hh"
#include "mqtt_topic.hh"
namespace mqtt{
    static u_int64_t TIMEOUT_MS = 10000;

    template <typename _TCP_>
    struct mqtt_session{
        struct data{
            _TCP_ inner_tcp;
            mqtt_pkt_connect session_info;
            std::list<subscribe_info> current_topic;
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

        ALWAYS_INLINE const char*
        get_id()const{
            return _inner_data_->session_info.data->client_id.c_str();
        }
        ALWAYS_INLINE bool
        has_clean_session(){
            return _inner_data_->session_info.data->check_clean_session();
        }
    };

    template <typename ..._T_>
    ALWAYS_INLINE static void
    ____throw_timeout(std::variant<int,_T_...>& v){
        switch (v.index()){
            case 0:
                throw std::logic_error("timeout");
            default:
                return;
        }
    }

    template <typename _TCP_IMPL_>
    auto
    wait_command(mqtt_session<_TCP_IMPL_>&& session){
        return session._inner_data_->inner_tcp.wait_packet(TIMEOUT_MS,1)
                .then([](FLOW_ARG(std::variant<int,common::ringbuffer*>)&& v)mutable{
                    ____forward_flow_monostate_exception(v);
                    auto&& d=std::get<std::variant<int,common::ringbuffer*>>(v);
                    ____throw_timeout(d);
                    return std::get<common::ringbuffer*>(d)->pop_uint_8();
                });
    }

    template <typename _TCP_IMPL_>
    engine::reactor::flow_builder<u_int32_t>
    wait_remaining(mqtt_session<_TCP_IMPL_>&& session,u_int32_t last,u_int32_t multiplier){
        if(multiplier>128*128*128)
            throw std::logic_error("error for read remaining");
        else
            return session._inner_data_->inner_tcp.wait_packet(TIMEOUT_MS,1)
                    .then([session=std::forward<mqtt_session<_TCP_IMPL_>>(session),last,multiplier](FLOW_ARG(std::variant<int,common::ringbuffer*>)&& v)mutable{
                        ____forward_flow_monostate_exception(v);
                        auto&& d=std::get<std::variant<int,common::ringbuffer*>>(v);
                        ____throw_timeout(d);
                        u_int32_t u=std::get<common::ringbuffer*>(d)->pop_uint_8();
                        u_int32_t r=last+((u&0b01111111)*multiplier);
                        if((u&0b10000000)!=0)
                            return wait_remaining(std::forward<mqtt_session<_TCP_IMPL_>>(session),last,multiplier*128);
                        else
                            return engine::reactor::make_imme_flow(std::forward<u_int32_t>(r));
                    });
    }

    template <typename _TCP_IMPL_>
    auto
    wait_packet(mqtt_session<_TCP_IMPL_>&& session){
        return wait_command(std::forward<mqtt_session<_TCP_IMPL_>>(session))
                .then([session=std::forward<mqtt_session<_TCP_IMPL_>>(session)](FLOW_ARG(u_int8_t)&& v)mutable{
                    ____forward_flow_monostate_exception(v);
                    return wait_remaining(std::forward<mqtt_session<_TCP_IMPL_>>(session),0,0)
                            .then([session=std::forward<mqtt_session<_TCP_IMPL_>>(session),cmd=std::get<u_int8_t>(v)](FLOW_ARG(u_int32_t)&& v)mutable{
                                ____forward_flow_monostate_exception(v);
                                u_int32_t remaining=std::get<u_int32_t>(v);
                                return session._inner_data_->inner_tcp.wait_packet(TIMEOUT_MS,remaining)
                                        .then([cmd,remaining](FLOW_ARG(std::variant<int,common::ringbuffer*>)&& v){
                                            ____forward_flow_monostate_exception(v);
                                            auto&& d=std::get<std::variant<int,common::ringbuffer*>>(v);
                                            ____throw_timeout(d);
                                            return std::make_tuple(cmd,remaining,std::get<common::ringbuffer*>(d));
                                        });
                            });
                });
    }

    template <typename _TCP_IMPL_>
    ALWAYS_INLINE auto
    send_packet(mqtt_session<_TCP_IMPL_>&& session,const char* buf,size_t len){
        return session._inner_data_->inner_tcp.send_packet(buf,len);
    }



}



#endif //PROJECT_MQTT_SESSION_H
