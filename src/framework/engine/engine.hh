//
// Created by anyone on 20-2-15.
//

#ifndef PROJECT_ENGINE_HH
#define PROJECT_ENGINE_HH

#include <signal.h>
#include <setjmp.h>
#include <string>
#include <f-stack/ff_api.h>
#include <dpdk/include/rte_ring.h>
#include <dpdk/include/rte_mempool.h>
#include <variant>
#include <common/ncpy_func.hh>
#include <engine/reactor/flow.hh>
#include <engine/reactor/schdule.hh>
#include <engine/timer/timer_set.hh>

namespace engine{

    constexpr size_t glb_msg_size=1024;

    constexpr const char* PRIMARY_RING_NAME= static_cast<const char*>("PRIMARY_RING");

    struct glb_context{
        rte_ring*                           primary_ring= nullptr;
        rte_mempool*                        primary_message_pool= nullptr;
        int                                 kqfd;
        kevent                              events[512];
    };

    jmp_buf on_ff_init;

    void
    install_sigsegv_handler(){
        struct sigaction sa;
        sa.sa_sigaction = [](int sig, siginfo_t *info, void *p) {
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
    void
    install_sigabrt_handler(){
        struct sigaction sa;
        sa.sa_sigaction = [](int sig, siginfo_t *info, void *p) {
            std::cout<<"SIGABRT error"<<std::endl;
            throw std::system_error();
        };
        sigfillset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
        auto r = ::sigaction(SIGABRT, &sa, nullptr);
        if(r == -1){
            throw std::system_error();
        }
    }

    auto
    wait_engine_initialled(int argc, char* argv[]){
        //install_sigsegv_handler();
        //install_sigabrt_handler();
        return engine::reactor::make_imme_flow()
                .then([argc,argv](FLOW_ARG()&& a){
                    if(ff_init(argc,argv)<0)
                        throw std::logic_error("cant init f-stack or dpdk");
                    auto p=std::make_shared<glb_context>();
                    if(rte_eal_process_type() == RTE_PROC_PRIMARY){
                        p->primary_ring=rte_ring_create(PRIMARY_RING_NAME,64,rte_socket_id(),0);
                        p->primary_message_pool= rte_mempool_create(PRIMARY_RING_NAME, 1024,
                                                                    glb_msg_size, 0, 0,
                                                                    NULL, NULL, NULL, NULL,
                                                                    rte_socket_id(), 0);
                    }
                    else{
                        p->primary_ring=rte_ring_lookup(PRIMARY_RING_NAME);
                        p->primary_message_pool=rte_mempool_lookup(PRIMARY_RING_NAME);
                    }
                    p->kqfd=ff_kqueue();
                    return p;
                });
    }
    template <typename _F_>
    auto engine_run(std::shared_ptr<glb_context> context,_F_&& f){
        ff_run(
                [](void* arg){
                    auto a=static_cast<std::tuple<std::shared_ptr<glb_context>,_F_>*>(arg);
                    auto [context,f]=*(a);
                    timespec t{0,0};
                    int nevents=ff_kevent(context->kqfd, nullptr, 0, context->events,512, &t);
                    for(int i=0;i<nevents;++i){
                        (*static_cast<common::ncpy_func<void(const kevent&)>*>(context->events[i].udata))(context->events[i]);
                    }
                    engine::timer::_sp_timer_set->handle_timeout(engine::timer::now_tick());
                    engine::reactor::_sp_global_task_center_->run();
                    f();
                    return 0;
                },
                new std::tuple<std::shared_ptr<glb_context>,_F_>(context,f)
        );
    }
}
#endif //PROJECT_ENGINE_HH
