cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)
set(BOOST_ROOT "/usr/local/include/boost")
set(PROJECT_ROOT "~/work/pump")

add_compile_options("-O3")
add_compile_options("-fnon-call-exceptions")

include_directories(${BOOST_ROOT}/include)
include_directories(${PROJECT_ROOT}/src)

add_subdirectory(src/app/mqtt)
add_subdirectory(src/app/echo)