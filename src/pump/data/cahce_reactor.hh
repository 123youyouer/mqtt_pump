//
// Created by null on 20-2-6.
//

#ifndef PROJECT_CAHCE_REACTOR_HH
#define PROJECT_CAHCE_REACTOR_HH

#include <hw/cpu.hh>
#include <data/cahce.hh>
#include <reactor/flow.hh>
namespace data{
    thread_local cache_lsu<int,std::string> cache_on_thread;
}
#endif //PROJECT_CAHCE_REACTOR_HH
