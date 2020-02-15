#include <iostream>
#include <memory>
#include <pump/reactor/task.hh>
#include <pump/reactor/flow.hh>

#include <pump/hw/cpu.hh>
#include <threading/threading.hh>
#include <sys/time.h>
#include <pump/poller/poller.hh>
#include <pump/net/listener.hh>
#include <pump/net/packet_buffer.hh>
#include <pump/net/tcp_session.hh>
#include <threading/reactor.hh>
#include <mqtt/mqtt_session.hh>
#include <pump/reactor/global_task_schedule_center.hh>
#include <timer/timer_set.hh>
#include <spdlog/spdlog.h>
#include <libaio.h>

void install_sigsegv_handler(){
    static utils::spinlock lock;
    struct sigaction sa;
    sa.sa_sigaction = [](int sig, siginfo_t *info, void *p) {
        std::lock_guard<utils::spinlock> g(lock);
        std::cout<<"SIGSEGV error"<<std::endl;
        throw std::system_error();
    };
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    auto r = ::sigaction(SIGSEGV, &sa, nullptr);
    if(r == -1){
        throw std::system_error();
    }
}


template <typename ..._T>
void init_all(){
    install_sigsegv_handler();
    timer::init_all_timer_set(hw::the_cpu_count);
    reactor::init_global_task_schedule_center<_T...>(hw::the_cpu_count);
    poller::init_all_poller(hw::the_cpu_count);

    for(int i=0;i<hw::the_cpu_count;++i){
        threading::make_thread<_T...>(hw::cpu_core(i)).detach();
    }

}

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




#include <async_file/aio_file.hh>
#include <pump/logger/logger.hh>
int main() {

    hw::pin_this_thread(0);
    sleep(1);
    hw::the_cpu_count=2;
    init_all
            <
                    reactor::sortable_task<utils::noncopyable_function<void()>,1>,
                    reactor::sortable_task<utils::noncopyable_function<void()>,2>
            >();
    sleep(1);

    logger::default_logger_ptr=new logger::simple_logger;
    logger::info("it is log {1} {0}",10,11);

/*
    aio::aio_file af("logs/t1.txt");

    af.start_at(hw::cpu_core::_01);
    char* sz=new char[6];
    sprintf(sz,"54321");

    af.wait_write_done(sz,5)
            .then([](aio::aio_cb_args&& a){
                std::cout<<a.res1<<" | "<<a.res2<<std::endl;
            },hw::cpu_core::_02)
            .submit();
    char buf[128];

    af.wait_read_done(buf,127,0)
            .then([&buf](aio::aio_cb_args&& a){
                std::cout<<a.res1<<" | "<<a.res2<<std::endl;
                std::cout<<buf<<std::endl;
            })
            .submit();

*/



    run_mqtt(net::listener<9022>::instance.start_at(hw::cpu_core::_01));
    reactor::make_flow([]() {
        std::cout <<"---------" << static_cast<int>(hw::get_thread_cpu_id()) <<"%%%%%%%%%%%%%" << std::endl;
        return 10;
    },hw::cpu_core::_02)
            .then([](int &&i) {
                std::cout <<"---------" << static_cast<int>(hw::get_thread_cpu_id()) << std::endl;
                return ++i;
            })
            .then([](int &&i) {
                std::cout <<"---------" << static_cast<int>(hw::get_thread_cpu_id()) << std::endl;
                return ++i;
            })
            .submit();

    sleep(10000);
    return 0;
}