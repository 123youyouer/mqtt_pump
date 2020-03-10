//
// Created by root on 2020/3/9.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "InfiniteRecursion"
#ifndef PUMP_MQTT_PROC_HH
#define PUMP_MQTT_PROC_HH
#include <system_error>
//#include <engine/net/tcp_session.hh>
#include "mqtt_packet.hh"
#include "handle_pingreq.hh"
#include "handle_connect.hh"
#include "handle_publish.hh"
#include "handle_subscribe.hh"
#include "handle_unsubscribe.hh"
#include "mqtt_session.hh"
namespace mqtt{
    template <typename _TCP_IMPL_>
    struct mqtt_proc{
        static void
        loop_session(mqtt_session<_TCP_IMPL_>&& session){
            wait_packet(std::forward<mqtt_session<_TCP_IMPL_>>(session))
                    .then([session=std::forward<mqtt_session<_TCP_IMPL_>>(session)](FLOW_ARG(std::tuple<u_int8_t,u_int32_t,common::ringbuffer*>)&& v)mutable{
                        ____forward_flow_monostate_exception(v);
                        auto [cmd,remaining,others]=std::get<std::tuple<u_int8_t,u_int32_t,common::ringbuffer*>>(v);
                        switch (mqtt_command_type(cmd&0xF0)){
                            case mqtt_command_type::connect:
                                throw std::logic_error("re-connect");
                            case mqtt_command_type::pingreq:
                                pong(std::forward<mqtt_session<_TCP_IMPL_>>(session));
                                break;
                            case mqtt_command_type::publish:
                                handle_publish(mqtt_pkt_publish(cmd,remaining,others));
                                break;
                            case mqtt_command_type::subscribe:
                                handle_subscribe(mqtt_pkt_subscribe(cmd,remaining,others));
                                break;
                            case mqtt_command_type::unsubscribe:
                                handle_unsubscribe(mqtt_pkt_unsubscribe(cmd,remaining,others));
                                break;
                            default:
                                throw std::logic_error("unknown command type");
                        }
                    })
                    .then([session=std::forward<mqtt_session<_TCP_IMPL_>>(session)](FLOW_ARG()&& v)mutable{
                        switch (v.index()){
                            case 0:
                                loop_session(std::forward<mqtt_session<_TCP_IMPL_>>(session));
                                return;
                            default:
                                session._inner_data_->inner_tcp.close();
                                return;
                        }
                    })
                    .submit();
        }

        static engine::reactor::flow_builder<>
        wait_connected(mqtt_session<_TCP_IMPL_>&& session){
            return wait_packet(std::forward<mqtt_session<_TCP_IMPL_>>(session))
                    .then([session=std::forward<mqtt_session<_TCP_IMPL_>>(session)](FLOW_ARG(std::tuple<u_int8_t,u_int32_t,common::ringbuffer*>)&& v)mutable{
                        ____forward_flow_monostate_exception(v);
                        auto [cmd,remaining,payload]=std::get<std::tuple<u_int8_t,u_int32_t,common::ringbuffer*>>(v);
                        switch (mqtt_command_type(cmd&0xF0)){
                            case mqtt_command_type::connect:
                                return handle_connect(std::forward<mqtt_session<_TCP_IMPL_>>(session),mqtt_pkt_connect(cmd,remaining,payload));
                            default:
                                throw std::logic_error("un connected!!");
                        }
                    });
        }
        static void
        mqtt_session_proc(mqtt_session<_TCP_IMPL_>&& session){
            wait_connected(std::forward<mqtt_session<_TCP_IMPL_>>(session))
                    .then([session=std::forward<mqtt_session<_TCP_IMPL_>>(session)](FLOW_ARG()&& v)mutable{
                        switch (v.index()){
                            case 0:
                                loop_session(std::forward<mqtt_session<_TCP_IMPL_>>(session));
                                return;
                            default:
                                session._inner_data_->inner_tcp.close();
                                return;
                        }
                    })
                    .submit();
        }
        ALWAYS_INLINE static void
        mqtt_session_proc(_TCP_IMPL_&& session){
            mqtt_session_proc(mqtt_session<_TCP_IMPL_>(std::forward<_TCP_IMPL_>(session)));
        }
    };
}
#endif //PUMP_MQTT_PROC_HH

#pragma clang diagnostic pop