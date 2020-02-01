//
// Created by null on 20-2-1.
//
#include <iostream>
#include <hw/cpu.hh>
#include <pump.hh>
#include <net/listener.hh>
#include <mqtt/mqtt_session.hh>


#pragma clang diagnostic push
#pragma ide diagnostic ignored "InfiniteRecursion"
template<uint16_t _PORT_>
void run_mqtt(net::listener<_PORT_>& l){
    net::wait_connect(l)
            .then([&l](net::session_data &&session_d) {
                net::tcp_session *s = new net::tcp_session(session_d);
                mqtt::wait_connect(s->start_at(hw::cpu_core::_01))
                        .then([s](mqtt::mqtt_parse_state_code &&code) {
                            switch (code) {
                                case mqtt::mqtt_parse_state_code::ok:
                                default:
                                    return;
                            }
                        })
                        .submit();
                run_mqtt(l);
            })
            .submit();
}
#pragma clang diagnostic pop

int main(){
    hw::pin_this_thread(0);
    sleep(1);
    hw::the_cpu_count=2;
    engine::init_engine
            <
                    reactor::sortable_task<utils::noncopyable_function<void()>,1>,
                    reactor::sortable_task<utils::noncopyable_function<void()>,2>
            >();
    sleep(1);
    logger::info("server started ......");

    run_mqtt(net::listener<9022>::instance.start_at(hw::cpu_core::_01));

    logger::info("server listen at {}",9022);

    sleep(10000);
    return 0;
}

