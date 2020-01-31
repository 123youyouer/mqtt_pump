//
// Created by null on 20-1-30.
//

#ifndef PROJECT_LOGGER_HH
#define PROJECT_LOGGER_HH

#include <moodycamel/concurrentqueue.h>
#include <hw/cpu.hh>

namespace log{
    class
    default_logger{
    private:
        uint8_t sink_flag;
        moodycamel::ConcurrentQueue<std::string_view> _q;
    public:
        inline void
        start_at(const hw::cpu_core& cpu){
        }
    public:
        inline void
        info(const char* sz){
        }
        inline void
        error(const char* sz){
        }
    };
    inline void info(){

    }
}
#endif //PROJECT_LOGGER_HH
