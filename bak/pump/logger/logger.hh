//
// Created by null on 20-1-30.
//

#ifndef PROJECT_LOGGER_HH
#define PROJECT_LOGGER_HH

#include "../hw/cpu.hh"
#include "../async_file/aio_file.hh"
#include "fmt/fmt.hh"

namespace logger{
    using string_view_t = fmt::basic_string_view<char>;
    using memory_buf_t = fmt::basic_memory_buffer<char, 250>;
    enum class log_level
    {
        trace = 1,
        debug = 2,
        info = 3,
        warn = 4,
        err = 5,
        critical = 6,
        off = 7,
    };
    class
    simple_logger{
    private:
        static const uint8_t sink_flag_file=1<<1;
        static const uint8_t sink_flag_console=1;
        uint8_t sink_flag;
        aio::aio_file _f;
    public:
        simple_logger():sink_flag(sink_flag_file|sink_flag_console),_f("log.txt"){
            //_f.start_at(hw::get_thread_cpu_id());
        }
        explicit
        simple_logger(uint8_t _sf):sink_flag(_sf),_f("log.txt"){
            //_f.start_at(hw::get_thread_cpu_id());
        }
        template<typename... Args>
        inline void
        log(log_level lvl, const char* sfmt, const Args &... args){
            auto s=fmt::format(sfmt,args...);
            if(sink_flag&sink_flag_console)
                std::cout<<s.data()<<std::endl;
            if(sink_flag&sink_flag_file)
                _f.write_no_res(s.data(),s.length());
        }
        template<typename... Args>
        void trace(const char* fmt, const Args &... args)
        {
            log(log_level::trace, fmt, args...);
        }

        template<typename... Args>
        void debug(const char* fmt, const Args &... args)
        {
            log(log_level::debug, fmt, args...);
        }

        template<typename... Args>
        void info(const char* fmt, const Args &... args)
        {
            log(log_level::info, fmt, args...);
        }

        template<typename... Args>
        void warn(const char* fmt, const Args &... args)
        {
            log(log_level::warn, fmt, args...);
        }

        template<typename... Args>
        void error(const char* fmt, const Args &... args)
        {
            log(log_level::err, fmt, args...);
        }

        template<typename... Args>
        void critical(const char* fmt, const Args &... args)
        {
            log(log_level::critical, fmt, args...);
        }
    };
    simple_logger* default_logger_ptr = nullptr;
    template<typename... Args>
    void trace(const char* fmt, const Args &... args)
    {
        default_logger_ptr->log(log_level::trace, fmt, args...);
    }

    template<typename... Args>
    void debug(const char* fmt, const Args &... args)
    {
        default_logger_ptr->log(log_level::debug, fmt, args...);
    }

    template<typename... Args>
    void info(const char* fmt, const Args &... args)
    {
        //default_logger_ptr->log(log_level::info, fmt, args...);
    }

    template<typename... Args>
    void warn(const char* fmt, const Args &... args)
    {
        default_logger_ptr->log(log_level::warn, fmt, args...);
    }

    template<typename... Args>
    void error(const char* fmt, const Args &... args)
    {
        default_logger_ptr->log(log_level::err, fmt, args...);
    }

    template<typename... Args>
    void critical(const char* fmt, const Args &... args)
    {
        default_logger_ptr->log(log_level::critical, fmt, args...);
    }
}
#endif //PROJECT_LOGGER_HH
