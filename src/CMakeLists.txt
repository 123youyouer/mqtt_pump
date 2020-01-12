project(pump)

set(CMAKE_C_FLAGS "-O3")
include_directories(${BOOST_ROOT}/include)
include_directories(${PROJECT_ROOT}/src)



add_executable(pump
        main.cpp
        reactor/task_channel.hh
        reactor/task_looper.hh
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
        reactor/flow.hh reactor/schedule.hh moodycamel/concurrentqueue.h net/schedule.hh net/session.hh reactor/global_task_schedule_center.hh net/packet_buffer.hh)

target_link_libraries(pump
        -lhwloc -pthread)

#target_compile_options (pump PUBLIC -fcoroutines-ts -g)