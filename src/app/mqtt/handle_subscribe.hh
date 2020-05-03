//
// Created by root on 2020/3/9.
//

#ifndef PUMP_HANDLE_SUBSCRIBE_HH
#define PUMP_HANDLE_SUBSCRIBE_HH

#include <boost/noncopyable.hpp>
#include "mqtt_packet.hh"
#include "mqtt_topic.hh"
namespace mqtt{
    template <typename _SESSION_>
    auto
    handle_subscribe(_SESSION_&& session,mqtt_pkt_subscribe&& pkt){
        return engine::reactor::make_task_flow()
                .then([session=std::forward<_SESSION_>(session),pkt=std::move(pkt)](FLOW_ARG()&& v)mutable{
                    ____forward_flow_monostate_exception(v);
                    for(auto e:pkt.data->topic_subscribe_infos)
                        subscribe(e->topic.c_str(),subscribe_info{rte_lcore_id(),session.get_id(),e->qos});
                    const char* buf= reinterpret_cast<const char*>(mqtt_pkt_suback::make_buf(pkt.data->packet_id));
                    return send_packet(std::forward<_SESSION_>(session),buf,mqtt_pkt_suback::len())
                            .then([buf](FLOW_ARG(std::tuple<const char*,size_t>)&& v){
                                delete [] buf;
                                ____forward_flow_monostate_exception(v);
                            });
                });
    }
}
#endif //PUMP_HANDLE_SUBSCRIBE_HH
