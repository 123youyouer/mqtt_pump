#include <iostream>
#include <boost/noncopyable.hpp>
#include <boost/circular_buffer.hpp>
#include <memory>
#include <reactor/task_channel.hh>
#include <reactor/task.hh>
#include <reactor/task_looper.hh>
#include <reactor/flow.hh>

#include <hw/cpu.hh>
#include <threading/threading.hh>
#include <sys/time.h>
#include <poller/poller.hh>
#include <net/listener.hh>
#include <net/packet_buffer.hh>
#include <net/session.hh>
#include <threading/reactor.hh>

#include <reactor/global_task_schedule_center.hh>

struct bbbb:boost::noncopyable{
    char* szbuf;
public:
    bbbb():szbuf(new char[128]){
        sprintf(szbuf,"000");
    }
};

struct nocpy:boost::noncopyable{
    char* szbuf;
    bbbb b;
public:
    void out(){
        std::cout<<szbuf<<std::endl;
    }
    nocpy(const char* sz):szbuf(new char[128]){
        sprintf(szbuf,sz);
    }
    nocpy(const nocpy&& n):szbuf(n.szbuf){
       // n.szbuf= nullptr;
    };
    nocpy(nocpy&& n):szbuf(n.szbuf){
        n.szbuf= nullptr;
    };
    virtual ~nocpy(){
        delete(szbuf);
    }
};

long getCurrentTime()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
void yyy(){

}


void run(net::listener& l){
    l.wait_connect()
            .and_then([&l](net::session_data&& session_d){
                net::session* s=new net::session(session_d);
                s->start_at(hw::cpu_core::_03)
                        .wait_packet()
                        .and_then([s](net::linear_ringbuffer_st* pkt){
                            std::cout<<"received packet :"<<pkt->read_head()<<std::endl;
                            size_t len=pkt->size();
                            return s->send_data(pkt->read_head(),pkt->size())
                                    .and_then([pkt,len](){
                                        std::cout<<"sended packet"<<std::endl;
                                        pkt->consume(len);
                                        return 1;
                                    });
                        })
                        .and_then([](int&& a){
                            std::cout<<"a="<<a<<std::endl;
                            return ++a;
                        })
                        .submit();
            })
            .submit();
}
template <typename ..._T>
void init_all(){
    reactor::init_global_task_schedule_center<_T...>(hw::the_cpu_count);
    poller::init_all_poller(hw::the_cpu_count);

    for(int i=0;i<hw::the_cpu_count;++i){
        threading::make_thread<_T...>(hw::cpu_core(i)).detach();
    }

}
void test2(){

}
int main() {
    hw::pin_this_thread(2);
    sleep(1);
    hw::the_cpu_count=3;
    init_all
            <
                    reactor::sortable_task<utils::noncopyable_function<void()>,1>,
                    reactor::sortable_task<utils::noncopyable_function<void()>,2>
            >();
    net::listener l=net::listener(9022);
    sleep(1);
    run(l.start_at(hw::cpu_core::_02));

    reactor::make_flow([]() { return 80; }, hw::cpu_core::_01)
            .and_then([](int&& x){
                int a=456;
                std::cout<<x<<std::endl;
                return "aaa";
            })
            .and_then([](std::string&& s){
                std::cout<<s<<std::endl;
                return reactor::make_flow([]() {
                    std::cout << "void:" << std::endl;
                    return 10;
                }, hw::cpu_core::_02)
                        .and_then([](int && a){
                            std::cout<<a<<std::endl;
                            return;
                        })
                        .and_then([](){
                            std::cout<<"00000"<<std::endl;
                            return;
                        });
            })
            .and_then([](){
                std::cout<<"112233"<<std::endl;
                return;
            },hw::cpu_core::_03)
            .submit();

    /*

    reactor::make_flow([](){
        return 10;
    })->at(hw::cpu_core::_03)
            ->and_then([](int&& a){
                std::cout<<"a="<<a<<std::endl;
                return ++a;
            })
            ->and_then([](int&& a){
                std::cout<<"a="<<a<<std::endl;
                return;
            })
            ->and_then([](){
                int a=123;
                std::cout<<"a="<<a<<std::endl;
                return reactor::make_flow([a=a]()mutable{
                    std::cout<<"a="<<a<<std::endl;
                    return ++a;
                })->at(hw::cpu_core::_02)
                        ->and_then([](int&& a){
                            std::cout<<"a="<<a<<std::endl;
                            return ++a;
                        })
                        ->and_then([](int&& a){
                            std::cout<<"a="<<a<<std::endl;
                            return ++a;
                        });
            })
            ->and_then([](int&& a){
                std::cout<<"a="<<a<<std::endl;
                return ++a;
            })
            ->submit();
*/
    sleep(10000);
    return 0;
}