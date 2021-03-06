//
// Created by null on 19-8-19.
//

#ifndef CPU_HH
#define CPU_HH

#include <hwloc.h>
#include <iostream>
#include <common/defer.hh>
#include <common/g_define.hh>
namespace pump::common{
    enum class cpu_core{
        _01=0,
        _02=1,
        _03=2,
        _04=3,
        _05=4,
        _06=5,
        _07=6,
        _08=7,
        _09=8,
        _10=9,
        _11=10,
        _12=11,
        _ANY=2000,
    };

    int
    nr_processing_units() {
        hwloc_topology_t topology;
        hwloc_topology_init(&topology);
        auto free_hwloc = common::defer([&] {
            hwloc_topology_destroy(topology);
        });
        hwloc_topology_load(topology);
        return hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
    }

    uint16_t the_cpu_count= static_cast<uint16_t>(nr_processing_units());

    ALWAYS_INLINE void
    pin_this_thread(unsigned int cpu_id) {
        cpu_set_t cs;
        CPU_ZERO(&cs);
        CPU_SET(cpu_id, &cs);
        try {
            auto t=pthread_self();
            auto r = pthread_setaffinity_np(t, sizeof(cs), &cs);
        }
        catch (...){
            std::cout<<"a"<<std::endl;
        }
    }
    ALWAYS_INLINE void
    pin_this_thread(const common::cpu_core& cpu) {
        return pin_this_thread(static_cast<unsigned int>(cpu));
    }

    ALWAYS_INLINE cpu_core
    get_thread_cpu_id(){
        cpu_set_t cs;
        CPU_ZERO(&cs);
        pthread_getaffinity_np(pthread_self(), sizeof(cs), &cs);
        int cpu_id=0;
        for (int i = 0; i < CPU_SETSIZE; i++)
            if (CPU_ISSET(i, &cs))
                return cpu_core(i);
        throw std::logic_error("unknown cpu");
    }
}
#endif //CPU_HH
