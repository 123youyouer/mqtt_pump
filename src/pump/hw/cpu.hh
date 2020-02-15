//
// Created by null on 19-8-19.
//

#ifndef CPPMQ_CPU_HH
#define CPPMQ_CPU_HH

#include <hwloc.h>
#include <pump/utils/defer.hh>
#include <iostream>
namespace hw{
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
        _ANY=2000
    };

    int nr_processing_units() {
        hwloc_topology_t topology;
        hwloc_topology_init(&topology);
        auto free_hwloc = defer([&] {
            hwloc_topology_destroy(topology);
        });
        hwloc_topology_load(topology);
        return hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
    }

    int the_cpu_count=nr_processing_units();

    inline
    void pin_this_thread(unsigned int cpu_id) {
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

    inline
    cpu_core get_thread_cpu_id(){
        cpu_set_t cs;
        CPU_ZERO(&cs);
        pthread_getaffinity_np(pthread_self(), sizeof(cs), &cs);
        int cpu_id=0;
        for (int i = 0; i < CPU_SETSIZE; i++)
            if (CPU_ISSET(i, &cs))
                return cpu_core(i);
    }
}

#endif //CPPMQ_CPU_HH
