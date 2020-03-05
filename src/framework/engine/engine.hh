//
// Created by anyone on 20-2-15.
//

#ifndef PROJECT_ENGINE_HH
#define PROJECT_ENGINE_HH

#include <signal.h>
#include <setjmp.h>
#include <string>
#include <boost/program_options.hpp>
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

    constexpr const char* PRIMARY_RING_NAME= static_cast<const char*>("PUBLIC_RING");

    struct glb_context{
        std::string                         used_cpu_cores;
        rte_ring*                           public_ring= nullptr;
        rte_mempool*                        public_message_pool= nullptr;
        rte_ring*                           this_ring= nullptr;
        rte_mempool*                        this_message_pool= nullptr;
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
                    int v=rte_log_register("PUMP");
                    rte_log(RTE_LOG_EMERG,RTE_LOGTYPE_USER1,"A%d",10);
                    boost::program_options::options_description arg_desc("arg");
                    arg_desc.add_options()
                            ("conf",boost::program_options::value<std::string>()->default_value("config.ini"),"set config");
                    boost::program_options::variables_map arg_vm;
                    store(boost::program_options::command_line_parser(argc, argv).options(arg_desc).allow_unregistered().run(), arg_vm);
                    notify(arg_vm);
                    if(arg_vm.count("conf")==0)
                        throw std::logic_error("can not find config file");

                    boost::program_options::options_description ini_desc("ini");
                    ini_desc.add_options()
                            ("pump.cpulist",boost::program_options::value<std::string>(),"used cpu core");
                    boost::program_options::variables_map ini_vm;
                    store(boost::program_options::parse_config_file<char>(arg_vm["config"].as<std::string>().c_str(),ini_desc,true),ini_vm);
                    if(ini_vm.count("pump.cpulist")==0)
                        throw std::logic_error("can not find cpu config");
                    return ini_vm["pump.cpulist"].as<std::string>();

                })
                .then([argc,argv](FLOW_ARG(std::string)&& a){
                    ____forward_flow_monostate_exception(a);

                    auto p=std::make_shared<glb_context>();
                    p->used_cpu_cores=std::get<std::string>(a);

                    if(ff_init(argc,argv)<0)
                        throw std::logic_error("cant init f-stack or dpdk");

                    if(rte_eal_process_type() == RTE_PROC_PRIMARY){
                        p->public_ring=rte_ring_create(PRIMARY_RING_NAME,64,rte_socket_id(),0);
                        p->public_message_pool= rte_mempool_create(PRIMARY_RING_NAME, 1024,
                                                                    glb_msg_size, 0, 0,
                                                                    NULL, NULL, NULL, NULL,
                                                                    rte_socket_id(), 0);
                    }
                    else{
                        p->public_ring=rte_ring_lookup(PRIMARY_RING_NAME);
                        p->public_message_pool=rte_mempool_lookup(PRIMARY_RING_NAME);
                    }
                    std::string s="PUMP";
                    s+=rte_lcore_id();
                    p->this_ring=rte_ring_create(s.c_str(),128,rte_socket_id(),0);
                    p->this_message_pool=rte_mempool_create(s.c_str(), 1024,
                                                            glb_msg_size, 0, 0,
                                                            NULL, NULL, NULL, NULL,
                                                            rte_socket_id(), 0);

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
