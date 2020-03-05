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
        template <typename _PKT_TYPES_>
        static engine::reactor::flow_builder<_PKT_TYPES_>
        wait_packet(mqtt_session<_TCP_IMPL_>&& session,_PKT_TYPES_&& pending){
            return session._inner_data_->inner_tcp.wait_packet(TIMEOUT_MS,pending.remaining_bytes())
                    .then([session=std::forward<mqtt_session<_TCP_IMPL_>>(session),pending=std::forward<_PKT_TYPES_>(pending)](FLOW_ARG(std::variant<int,common::ringbuffer*>)&& v)mutable{
                        ____forward_flow_monostate_exception(v);
                        auto&& d=std::get<std::variant<int,common::ringbuffer*>>(v);
                        switch (d.index()){
                            case 0:
                                session._inner_data_->inner_tcp.close();
                                throw std::logic_error("timeout");
                            default:
                                auto buf=std::get<common::ringbuffer*>(d);
                                switch (pending.build_pkt(buf)){
                                    case -1:
                                        session._inner_data_->inner_tcp.close();
                                        throw std::logic_error("build_pkt mqtt_pkt_connect return -1");
                                    case 0:
                                        return wait_packet(std::forward<mqtt_session<_TCP_IMPL_>>(session),std::forward<_PKT_TYPES_>(pending));
                                    default:
                                        return engine::reactor::make_imme_flow()
                                                .then([pkt=std::forward<_PKT_TYPES_>(pending)](FLOW_ARG()&& v){
                                                    return std::move(pkt);
                                                });
                                }
                        }
                    });
        }
        static void
        loop_session(mqtt_session<_TCP_IMPL_>&& session){
            session._inner_data_->inner_tcp.wait_packet(TIMEOUT_MS,2)
                    .then([session=std::forward<mqtt_session<_TCP_IMPL_>>(session)](FLOW_ARG(std::variant<int,common::ringbuffer*>)&& v)mutable{
                        ____forward_flow_monostate_exception(v);
                        auto&& d=std::get<std::variant<int,common::ringbuffer*>>(v);
                        switch (d.index()){
                            case 0:
                                session._inner_data_->inner_tcp.close();
                                throw std::logic_error("timeout");
                            default:
                                auto buf=std::get<common::ringbuffer*>(d);
                                switch (mqtt_command_type(buf->pop_uint_8()&0xF0)){
                                    case mqtt_command_type::connect:
                                        throw std::logic_error("re connect");
                                    case mqtt_command_type::pingreq:
                                        wait_packet(std::forward<mqtt_session<_TCP_IMPL_>>(session),mqtt_pkt_pingreq())
                                                .then([](FLOW_ARG(mqtt_pkt_pingreq)&& v){
                                                    ____forward_flow_monostate_exception(v);
                                                    mqtt_pkt_pingreq pkt=std::get<mqtt_pkt_pingreq>(v);
                                                })
                                                .submit();
                                        return;
                                    default:
                                        throw std::logic_error("unknown command type");
                                }
                        }
                    })
                    .then([session=std::forward<mqtt_session<_TCP_IMPL_>>(session)](FLOW_ARG()&& v)mutable{
                        switch (v.index()){
                            case 0:
                                loop_session(std::forward<mqtt_session<_TCP_IMPL_>>(session));
                            default:
                                try {
                                    std::rethrow_exception(std::get<std::exception_ptr>(v));
                                }
                                catch (...){

                                }
                        }
                    })
                    .submit();
        }

        static engine::reactor::flow_builder<mqtt_pkt_connect>
        wait_connected(mqtt_session<_TCP_IMPL_>&& session){
            return session._inner_data_->inner_tcp.wait_packet(TIMEOUT_MS,2)
                    .then([session=std::forward<mqtt_session<_TCP_IMPL_>>(session)](FLOW_ARG(std::variant<int,common::ringbuffer*>)&& v)mutable{
                        ____forward_flow_monostate_exception(v);
                        auto&& d=std::get<std::variant<int,common::ringbuffer*>>(v);
                        switch (d.index()){
                            case 0:
                                session._inner_data_->inner_tcp.close();
                                throw std::logic_error("timeout");
                            default:
                                auto buf=std::get<common::ringbuffer*>(d);
                                switch (mqtt_command_type(buf->pop_uint_8()&0xF0)){
                                    case mqtt_command_type::connect:
                                        return wait_packet(std::forward<mqtt_session<_TCP_IMPL_>>(session),mqtt_pkt_connect());
                                    default:
                                        throw std::logic_error("unconnected");
                                }
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
                    .then([](FLOW_ARG()&& v){
                        return;
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
