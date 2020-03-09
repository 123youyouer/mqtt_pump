//
// Created by root on 20-2-26.
//

#ifndef PROJECT_MQTT_SESSION_H
#define PROJECT_MQTT_SESSION_H
#include <system_error>
#include <engine/net/tcp_session.hh>
#include "mqtt_packet.hh"
#include "handle_pingreq.hh"
#include "handle_connect.hh"

namespace mqtt{
#pragma clang diagnostic push
#pragma ide diagnostic ignored "InfiniteRecursion"
#define TIMEOUT_MS 10000

    template <typename _TCP_>
    struct mqtt_session{
        struct data{
            _TCP_ inner_tcp;
            explicit
            data(_TCP_&& tcp):inner_tcp(std::forward<_TCP_>(tcp)){
            }
        };
        std::shared_ptr<data> _inner_data_;
        explicit
        mqtt_session(_TCP_&& impl):_inner_data_(std::make_shared<data>(std::forward<_TCP_>(impl))){}
    };

    template <typename _TCP_IMPL_>
    struct operation{
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
        static auto
        wait_command(mqtt_session<_TCP_IMPL_>&& session){
            return session._inner_data_->inner_tcp.wait_packet(TIMEOUT_MS,1)
                    .then([](FLOW_ARG(std::variant<int,common::ringbuffer*>)&& v)mutable{
                        ____forward_flow_monostate_exception(v);
                        auto&& d=std::get<std::variant<int,common::ringbuffer*>>(v);
                        ____throw_timeout(d);
                        return std::get<common::ringbuffer*>(d)->pop_uint_8();
                    });
        }

        static engine::reactor::flow_builder<u_int32_t>
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
        static auto
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
        static void
        loop_session(mqtt_session<_TCP_IMPL_>&& session){
            wait_packet(std::forward<mqtt_session<_TCP_IMPL_>>(session))
                    .then([session=std::forward<mqtt_session<_TCP_IMPL_>>(session)](FLOW_ARG(std::tuple<u_int8_t,u_int32_t,common::ringbuffer*>)&& v)mutable{
                        ____forward_flow_monostate_exception(v);
                        auto [cmd,remaining,payload]=std::get<std::tuple<u_int8_t,u_int32_t,common::ringbuffer*>>(v);
                        switch (mqtt_command_type(cmd&0xF0)){
                            case mqtt_command_type::connect:
                                throw std::logic_error("re-connect");
                            case mqtt_command_type::pingreq:
                                loop_session(std::forward<mqtt_session<_TCP_IMPL_>>(session));
                            default:
                                throw std::logic_error("unknown command type");
                        }
                    })
                    .then([session=std::forward<mqtt_session<_TCP_IMPL_>>(session)](FLOW_ARG()&& v){
                        switch (v.index()){
                            case 0:
                                return;
                            default:
                                session._inner_data_->inner_tcp.close();
                                return;
                        }
                    })
                    .submit();
        }

        static engine::reactor::flow_builder<mqtt_pkt_connect>
        wait_connected(mqtt_session<_TCP_IMPL_>&& session){
            return wait_packet(std::forward<mqtt_session<_TCP_IMPL_>>(session))
                    .then([session=std::forward<mqtt_session<_TCP_IMPL_>>(session)](FLOW_ARG(std::tuple<u_int8_t,u_int32_t,common::ringbuffer*>)&& v){
                        ____forward_flow_monostate_exception(v);
                        auto [cmd,remaining,payload]=std::get<std::tuple<u_int8_t,u_int32_t,common::ringbuffer*>>(v);
                        switch (mqtt_command_type(cmd&0xF0)){
                            case mqtt_command_type::connect:
                                return mqtt_pkt_connect(remaining,payload);
                            default:
                                throw std::logic_error("un connected!!");
                        }
                    });
        }
        static void
        mqtt_session_proc(mqtt_session<_TCP_IMPL_>&& session){
            wait_connected(std::forward<mqtt_session<_TCP_IMPL_>>(session))
                    .then([session=std::forward<mqtt_session<_TCP_IMPL_>>(session)](FLOW_ARG(mqtt_pkt_connect)&& v)mutable{
                        ____forward_flow_monostate_exception(v);
                        handle_connect(std::get<mqtt_pkt_connect>(v));
                        loop_session(std::forward<mqtt_session<_TCP_IMPL_>>(session));
                    })
                    .then([session=std::forward<mqtt_session<_TCP_IMPL_>>(session)](FLOW_ARG()&& v)mutable{
                        switch (v.index()){
                            case 0:
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
#pragma clang diagnostic pop
}



#endif //PROJECT_MQTT_SESSION_H
