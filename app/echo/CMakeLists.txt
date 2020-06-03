project(echo)

include_directories(${PROJECT_ROOT}/src/app)
#include_directories(${PROJECT_ROOT}/src/3rd/freebsd_wrapper)
#include_directories(${PROJECT_ROOT}/src/3rd/freebsd)

link_directories(${PROJECT_ROOT}/3rd/dpdk)
link_directories(${PROJECT_ROOT}/3rd/f-stack)

add_compile_options("-O0")
add_compile_options(-mfma)
add_compile_options(-v)

add_executable(echo
        main.cc ee.hh)

target_link_libraries(echo
        -lhwloc
        ${LIBCXXLIBS}
        boost_program_options
        ${DPDKLIBS}
        pthread
        rt
        dl
        m
        numa
        crypto
        )