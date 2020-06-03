//
// Created by root on 2020/5/21.
//

#ifndef PUMP_PUMP_HH
#define PUMP_PUMP_HH

//#include <epoll/proc/task_proc.hh>
#include <net/wait_packet.hh>
namespace pump::epoll{
    using namespace std;
    struct AAA{
        //std::string empty(){ return "a";}
        void schedule(net::recv_handler<common::ringbuffer>&& a){}
        AAA()=default;
    };

    void test_pump(){
        //proc::task_proc_context ctx;
        //proc::task_proc(ctx);
        int i=10;
        variant<vector<int,std::allocator<int>>> v(std::in_place_index<0>,{1,2,3,4},std::allocator<int>());

        constexpr variant<int,bool> c(10);
        //c.emplace<0>(10);
        AAA a;
        std::shared_ptr<common::ringbuffer> pb;
        std::shared_ptr<AAA> pa;

        //auto l=net::wait_packet(pb,pa);
        cout<<11<<endl;
    }
}
#endif //PUMP_PUMP_HH
