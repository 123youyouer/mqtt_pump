//
// Created by null on 20-2-1.
//

#ifndef PROJECT_PUMP_HH
#define PROJECT_PUMP_HH

#include <pump/utils/spinlock.hh>
#include <sys/time.h>
#include <pump/hw/cpu.hh>
#include <pump/poller/poller.hh>
#include <pump/logger/logger.hh>
#include <mutex>
#include <signal.h>
#include <pump/timer/timer_set.hh>
#include <pump/threading/threading.hh>
namespace engine{
    void
    install_sigsegv_handler(){
        static common::spinlock lock;
        struct sigaction sa;
        sa.sa_sigaction = [](int sig, siginfo_t *info, void *p) {
            std::lock_guard<common::spinlock> g(lock);
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
    void
    init_engine(){
        install_sigsegv_handler();
        timer::init_all_timer_set(hw::the_cpu_count);
        reactor::init_global_task_schedule_center<_T...>(hw::the_cpu_count);
        poller::init_all_poller(hw::the_cpu_count);
        for(int i=0;i<hw::the_cpu_count;++i)
            threading::make_thread<_T...>(hw::cpu_core(i)).detach();
        logger::default_logger_ptr=new logger::simple_logger;
        for(int i=0;i<hw::the_cpu_count;++i)
            threading::_all_thread_state_[i].wait_start_flag=1;
    }
}

#endif //PROJECT_PUMP_HH
