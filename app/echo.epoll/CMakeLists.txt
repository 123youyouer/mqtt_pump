project(echo_epoll)

include_directories(${PROJECT_ROOT}/src/app)

add_compile_options("-O0")
add_compile_options(-mfma)
add_compile_options(-v)

add_executable(echo_epoll
        main.cc)

target_link_libraries(echo_epoll
        -lhwloc
        ${LIBCXXLIBS}
        pthread
        rt
        dl
        m
        numa
        crypto
        )