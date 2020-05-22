//
// Created by root on 2020/3/3.
//

#ifndef PUMP_HANDLE_CONNECT_HH
#define PUMP_HANDLE_CONNECT_HH

#include <boost/noncopyable.hpp>
#include "mqtt_cache.hh"
#include "mqtt_packet.hh"
#include "reactor/flow.hh"

namespace mqtt{
    template <typename _SESSION_>
    auto
    handle_connect(_SESSION_&& session,const mqtt_pkt_connect& pkt){
        session.set_connect_info(pkt);
        return reactor::make_task_flow()
                .then([session=std::forward<_SESSION_>(session)](FLOW_ARG()&& v)mutable{
                    ____forward_flow_monostate_exception(v);
                    gb_cache<const char*,_SESSION_>.push(session.get_id(),std::make_shared<_SESSION_>(std::forward<_SESSION_>(session)));
                    const char* buf= reinterpret_cast<const char*>(mqtt_pkt_connack::make_buf(session.has_clean_session()?0b00000001:0,0));
                    return send_packet(std::forward<_SESSION_>(session),buf,mqtt_pkt_connack::len())
                            .then([buf](FLOW_ARG(std::tuple<const char*,size_t>)&& v){
                                delete [] buf;
                                ____forward_flow_monostate_exception(v);
                            });
                });
    }
}
#endif //PUMP_HANDLE_CONNECT_HH
