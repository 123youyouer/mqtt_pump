//
// Created by root on 2020/3/9.
//

#ifndef PUMP_MQTT_CACHE_HH
#define PUMP_MQTT_CACHE_HH

#include <list>
#include <engine/data/cahce.hh>
namespace mqtt{
    template <typename _K_,typename _V_>
    engine::data::cache<_K_,_V_> gb_cache;
}
#endif //PUMP_MQTT_CACHE_HH
