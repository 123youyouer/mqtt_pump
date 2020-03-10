//
// Created by root on 2020/3/9.
//

#ifndef PUMP_HANDLE_UNSUBSCRIBE_HH
#define PUMP_HANDLE_UNSUBSCRIBE_HH
#include <boost/noncopyable.hpp>
#include "mqtt_packet.hh"
namespace mqtt{
    template <typename _SESSION_>
    auto
    handle_unsubscribe(_SESSION_&& session,const mqtt_pkt_unsubscribe& pkt){
        return engine::reactor::make_task_flow()
                .then([session=std::forward<_SESSION_>(session),pid=pkt.data->packet_id](FLOW_ARG()&& v)mutable{
                    ____forward_flow_monostate_exception(v);
                    const char* buf= reinterpret_cast<const char*>(mqtt_pkt_unsuback::make_buf(pid));
                    return send_packet(std::forward<_SESSION_>(session),buf,mqtt_pkt_unsuback::len())
                            .then([buf](FLOW_ARG(std::tuple<const char*,size_t>)&& v){
                                delete [] buf;
                                ____forward_flow_monostate_exception(v);
                            });
                });
    }
}
#endif //PUMP_HANDLE_UNSUBSCRIBE_HH
