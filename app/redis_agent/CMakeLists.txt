project(redis_agent)

include_directories(${PROJECT_ROOT}/app)
include_directories(${PROJECT_ROOT}/3rd/dpdk/include)
include_directories(${PROJECT_ROOT}/3rd)
include_directories(/home/anyone/libroot/googletest/googletest/include)

link_directories(${PROJECT_ROOT}/3rd/dpdk)
link_directories(${PROJECT_ROOT}/3rd/f-stack)
link_directories(${PROJECT_ROOT}/3rd/f-stack)
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
        resped/resp_decoder.hh
        resped/resp_command.hh
        ../../src/common/consistent_hashing.hh
        redis/redis_nodes.hh
        command/redis_command.hh
        command/redis_command_name.hh)

add_executable(redis_agent
        main.cc
        ${source}
        )

add_executable(redis_agent_test
        ${source}
        test/resp_test.hh
        test/test.cc)

target_link_libraries(redis_agent
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

target_link_libraries(redis_agent_test
        -lhwloc
        ${LIBCXXLIBS}
        boost_program_options
        gtest
        ${DPDKLIBS}
        pthread
        rt
        dl
        m
        numa
        crypto)