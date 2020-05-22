project(redis_agent)

include_directories(${PROJECT_ROOT}/src/app)
include_directories(${PROJECT_ROOT}/src/3rd/dpdk/include)
include_directories(${PROJECT_ROOT}/src/3rd)
include_directories(/home/anyone/libroot/googletest/googletest/include)
#include_directories(${PROJECT_ROOT}/src/3rd/freebsd_wrapper)
#include_directories(${PROJECT_ROOT}/src/3rd/freebsd)

link_directories(${PROJECT_ROOT}/src/3rd/dpdk)
link_directories(${PROJECT_ROOT}/src/3rd/f-stack)
link_directories(${PROJECT_ROOT}/src/3rd/f-stack)
link_directories(/home/anyone/libroot/googletest/build/lib)

add_compile_options("-O0")
add_compile_options(-mfma)
#add_compile_options(-fno-elide-constructors)
#add_compile_options(-gdwarf-2)

set(source
        command/redis_set.hh
        redis/redis_session.hh
        game/game_unit.hh
        agent/agent_session.hh
        resped/resp_decoder.hh)

add_executable(redis_agent
        ${source}
        main.cc resped/resp_command.hh ../../framework/common/consistent_hashing.hh redis/redis_nodes.hh command/redis_command.hh command/redis_command_name.hh)

add_executable(redis_agent_test
        ${source}
        test/resp_test.hh
        test/test.cc)

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

target_link_libraries(redis_agent_test
        boost_program_options
        gtest
        ${DPDKLIBS}
        pthread
        rt
        dl
        m
        numa
        crypto)