project(pump)
cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE on)
add_compile_options("-fnon-call-exceptions")

if(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    add_compile_options("-stdlib=libc++")
    add_compile_options("-nostdinc++")
    set(Boost_INCLUDE_DIRS /home/anyone/libroot/boost/1.73.src)
    set(Boost_LIBRARY_DIRS /home/anyone/libroot/boost/1.73.src/stage/lib)
    set(LIBCXXDIRS "/home/anyone/libroot/libc++/lib")
    set(LIBCXXLIBS -lc++ -lc++abi -lm -lc -lgcc_s -lgcc)
    include_directories(/home/anyone/libroot/clang/include/c++/v1)
else()
    find_package(Boost)
endif()

set(PROJECT_ROOT "~/work/pump")

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PROJECT_ROOT}/src)
include_directories(${PROJECT_ROOT}/3rd/dpdk/include)
include_directories(${PROJECT_ROOT}/3rd)

link_directories(${Boost_LIBRARY_DIRS})
link_directories(${LIBCXXDIRS})

set(DPDKLIBS
        -Wl,--whole-archive
        fstack
        dpdk
        -Wl,--no-whole-archive)

add_subdirectory(app/mqtt)
add_subdirectory(app/echo)
add_subdirectory(app/echo.epoll)
add_subdirectory(app/redis_agent)