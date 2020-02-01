project(mqtt_pump)

include_directories(${PROJECT_ROOT}/src/app)


add_executable(mqtt_pump
        main.cc
        #../pump/logger/fmt/format.cc
        )

target_link_libraries(mqtt_pump
        -lhwloc -pthread -laio)