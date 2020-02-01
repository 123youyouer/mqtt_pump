project(pump)

include_directories(${BOOST_ROOT}/include)
include_directories(${PROJECT_ROOT}/src)

add_compile_options("-O0")
add_compile_options("-fnon-call-exceptions")

add_executable(pump
        main.cpp
        hw/cpu.hh
        utils/defer.hh
        reactor/task.hh
        utils/can_inherit.hh
        utils/any.hh
        threading/threading.hh
        poller/poller_epoll.hh
        poller/poller.hh
        net/listener.hh
        net/echo_session.hh
        threading/reactor.hh
        reactor/flow.hh
        reactor/schedule.hh
        moodycamel/concurrentqueue.h
        net/schedule.hh
        net/tcp_session.hh
        reactor/global_task_schedule_center.hh
        net/packet_buffer.hh
        timer/timer_set.hh
        threading/threading_state.hh
        reactor/flow_state.hh
        utils/spinlock.hh
        utils/temp.hh
        logger/logger.hh
        async_file/aio_file.hh)

target_link_libraries(pump
        -lhwloc -pthread -laio)

#target_compile_options (pump PUBLIC -fcoroutines-ts -g)