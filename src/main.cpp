#include <iostream>
#include <memory>
#include <reactor/task.hh>
#include <reactor/flow.hh>

#include <hw/cpu.hh>
#include <threading/threading.hh>
#include <sys/time.h>
#include <poller/poller.hh>
#include <net/listener.hh>
#include <net/packet_buffer.hh>
#include <net/tcp_session.hh>
#include <threading/reactor.hh>
#include <mqtt/mqtt_session.hh>
#include <reactor/global_task_schedule_center.hh>
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


void
aio_cb(io_context_t ctx, struct iocb *iocb, long res, long res2){}


int main() {


    hw::pin_this_thread(0);
    sleep(1);
    hw::the_cpu_count=1;
    init_all
            <
                    reactor::sortable_task<utils::noncopyable_function<void()>,1>,
                    reactor::sortable_task<utils::noncopyable_function<void()>,2>
            >();
    sleep(1);


    int fd=open("logs/t.txt",O_RDWR|O_CREAT|O_APPEND,0664);
    int efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    io_context_t ctx;
    io_setup(1024,&ctx);
    struct iocb *io = (struct iocb *)malloc(sizeof(struct iocb));
    char* sz="12345";
    io_prep_pwrite(io,fd,sz,6,0);
    io_set_eventfd(io,efd);
    io_set_callback(io,[](io_context_t ctx, struct iocb *iocb, long res, long res2){
        std::cout << "00000" << std::endl;
    });
    io_submit(ctx,1,&io);

    struct io_event events[1024];

    int num=io_getevents(ctx,1,1024,events, nullptr);
    for (int i = 0; i < num; i++) {
        auto cb = (io_callback_t) events[i].data;
        struct iocb *io = events[i].obj;

        printf("events[%d].data = %x, res = %d, res2 = %d n", i, cb, events[i].res, events[i].res2);
        cb(ctx, io, events[i].res, events[i].res2);
    }



    run_mqtt(net::listener<9022>::instance.start_at(hw::cpu_core::_01));
    reactor::make_flow([]() {
        return 10;
    })
            .then([](int &&i) {
                std::cout << i << std::endl;
                return ++i;
            })
            .then([](int &&i) {
                std::cout << i << std::endl;
                return ++i;
            })
            .submit();

    sleep(10000);
    return 0;
}