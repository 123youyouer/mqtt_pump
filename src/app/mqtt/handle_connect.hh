//
// Created by root on 2020/3/3.
//

#ifndef PUMP_HANDLE_CONNECT_HH
#define PUMP_HANDLE_CONNECT_HH

#include <boost/noncopyable.hpp>
#include "mqtt_cache.hh"
#include "mqtt_packet.hh"
#include "engine/reactor/flow.hh"

namespace mqtt{
    template <typename _SESSION_>
    auto
    handle_connect(_SESSION_&& session,mqtt_pkt_connect& pkt){
        session.set_connect_info(pkt);
        return engine::reactor::make_task_flow()
                .then([session=std::forward<_SESSION_>(session)](FLOW_ARG()&& v)mutable{
                    ____forward_flow_monostate_exception(v);
                    gb_cache<const char*,_SESSION_>.push(session.get_id(),std::make_shared<_SESSION_>(std::forward<_SESSION_>(session)));
                    char* sz=new char[4];
                    return session.send_packet(sz,4)
                            .then([sz](FLOW_ARG()&& v){
                                ____forward_flow_monostate_exception(v);
                                delete [] sz;
                            });
                });

    }
}
#endif //PUMP_HANDLE_CONNECT_HH
