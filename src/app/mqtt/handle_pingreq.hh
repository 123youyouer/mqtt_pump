//
// Created by root on 2020/3/3.
//

#ifndef PUMP_HANDLE_PINGREQ_HH
#define PUMP_HANDLE_PINGREQ_HH

#include "mqtt_packet.hh"

namespace mqtt{
    template <typename _SESSION_>
    auto
    pong(_SESSION_&& session){
        return reactor::make_task_flow()
                .then([session=std::forward<_SESSION_>(session)](FLOW_ARG()&& v)mutable{
                    ____forward_flow_monostate_exception(v);
                    const char* buf= reinterpret_cast<const char*>(mqtt_pkt_pong::make_buf());
                    return send_packet(std::forward<_SESSION_>(session),buf,mqtt_pkt_pong::len())
                            .then([buf](FLOW_ARG(std::tuple<const char*,size_t>)&& v){
                                delete [] buf;
                                ____forward_flow_monostate_exception(v);
                            });
                });
    }
}
#endif //PUMP_HANDLE_PINGREQ_HH
