//
// Created by root on 2020/5/25.
//
#include <epoll/task_proc.hh>
#include <epoll/global/global_task_center.hh>
#include <timer/time_set.hh>
#include <epoll/net/poller_epoll.hh>
#include <epoll/net/tcp_session.hh>
#include <net/wait_connect.hh>
#include <net/send_packet.hh>
void test1(){
    std::cout<<3<<std::endl;
}
void test2(){
    std::cout<<4<<std::endl;
}
int main(int argc, char * argv[]){
    sp<pump::reactor::flow_runner> tasks(new pump::epoll::global::epoll_global_task_center());
    sp<pump::timer::time_set> times(new pump::timer::time_set());
    sp<pump::epoll::net::poller_epoll> polle(new pump::epoll::net::poller_epoll());

    pump::reactor::flow_builder<>::at_schedule(tasks)
            .then([](FLOW_ARG()&& a){
                std::cout<<1<<std::endl;
            })
            .then([](FLOW_ARG()&& a){
                std::cout<<2<<std::endl;
                int i=1;
                if(i)
                    test1(),test2();
            })
            .submit();

    pump::epoll::start_proc(times,polle,std::dynamic_pointer_cast<pump::epoll::global::epoll_global_task_center>(tasks))
            .run_at(pump::common::cpu_core::_03);
}
