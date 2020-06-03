project(mqtt_pump)

include_directories(${PROJECT_ROOT}/app)
include_directories(${PROJECT_ROOT}/3rd/dpdk/include)
include_directories(${PROJECT_ROOT}/src/3rd)
link_directories(${PROJECT_ROOT}/3rd/dpdk)
link_directories(${PROJECT_ROOT}/3rd/f-stack)
add_compile_options("-O0")
add_compile_options(-mfma)
#add_compile_options(-fno-elide-constructors)
#add_compile_options(-gdwarf-2)

add_executable(mqtt_pump
        main.cc
        #../epoll/logger/fmt/format.cc
        mqtt_session.hh)

set(DPDKLIBS
        -Wl,--whole-archive
        fstack
        dpdk
        -Wl,--no-whole-archive)

target_link_libraries(mqtt_pump
        -lhwloc
        ${LIBCXXLIBS}
        boost_program_options
        ${DPDKLIBS}
        pthread
        rt
        dl
        m
        numa
        crypto)