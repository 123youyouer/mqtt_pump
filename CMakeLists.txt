cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)
set(BOOST_ROOT "/home/anyone/libroot/boost/1.73")
set(PROJECT_ROOT "~/work/pump")
set(CMAKE_VERBOSE_MAKEFILE on)

add_compile_options("-fnon-call-exceptions")
#add_compile_options("-march=x86-64")
include_directories(${BOOST_ROOT}/include)
include_directories(${PROJECT_ROOT}/src/framework)
include_directories(${PROJECT_ROOT}/src/3rd)

add_subdirectory(src/app/mqtt)
add_subdirectory(src/app/echo)
add_subdirectory(src/app/redis_agent)