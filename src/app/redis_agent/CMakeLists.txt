project(redis_agent)

include_directories(${PROJECT_ROOT}/src/app)
include_directories(${PROJECT_ROOT}/src/3rd/dpdk/include)
include_directories(${PROJECT_ROOT}/src/3rd)
#include_directories(${PROJECT_ROOT}/src/3rd/freebsd_wrapper)
#include_directories(${PROJECT_ROOT}/src/3rd/freebsd)

link_directories(${PROJECT_ROOT}/src/3rd/dpdk)
link_directories(${PROJECT_ROOT}/src/3rd/f-stack)
link_directories(${PROJECT_ROOT}/src/3rd/f-stack)

add_compile_options("-O0")
add_compile_options(-mfma)
#add_compile_options(-fno-elide-constructors)
#add_compile_options(-gdwarf-2)

add_executable(redis_agent
        main.cc command/redis_set.hh session/redis_session.hh game/game_unit.hh session/agent_session.hh resped/resp_decoder.hh)

set(DPDKLIBS
        -Wl,--whole-archive
        fstack
        dpdk
        -Wl,--no-whole-archive)

target_link_libraries(redis_agent
        boost_program_options
        ${DPDKLIBS}
        pthread
        rt
        dl
        m
        numa
        crypto)