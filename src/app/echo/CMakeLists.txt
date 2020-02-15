project(echo)

include_directories(${PROJECT_ROOT}/src/app)


add_executable(echo
        main.cc
        )

target_link_libraries(echo
        -lhwloc -pthread -laio -lboost_program_options)