//
// Created by root on 2020/3/10.
//

#ifndef PUMP_MQTT_TOPIC_HH
#define PUMP_MQTT_TOPIC_HH
namespace mqtt{
    struct subscribe_info{
        u_int32_t cpu_id;
        std::string client_id;
        u_int32_t client_id_hash;
    };
    using gb_cache_type_subscribe=std::list<subscribe_info>;

    engine::data::cache<const char*,gb_cache_type_subscribe> topic_cache;

    auto
    subscribe(const char* topic,subscribe_info& who){
        auto r=topic_cache.find(topic);
        switch(r.index()){
            case 0:{
                auto v=std::make_shared<gb_cache_type_subscribe>();
                v->push_back(who);
                topic_cache.push(topic,v);
                return;
            }
            case 1:{
                auto v=std::get<std::shared_ptr<gb_cache_type_subscribe>>(r);
                v->push_back(who);
                return;
            }
        }
    }
    auto
    unsubscribe(const char* topic,subscribe_info& who){
    }
}
#endif //PUMP_MQTT_TOPIC_HH
