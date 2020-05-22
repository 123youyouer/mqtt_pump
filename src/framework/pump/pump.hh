//
// Created by root on 2020/5/21.
//

#ifndef PUMP_PUMP_HH
#define PUMP_PUMP_HH

#include <pump/proc/task_proc.hh>
#include <pump/net/tcp_session.hh>
namespace pump{
    struct AAA{
        //std::string empty(){ return "a";}
        bool empty(){ return false;}
        std::string aaa1(){ return "a";}
    };

    void test_pump(){
        //proc::task_proc_context ctx;
        //proc::task_proc(ctx);

        AAA a;
        auto l=net::wait_packet(a);
        std::cout<<11<<std::endl;
    }
}
#endif //PUMP_PUMP_HH
