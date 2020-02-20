//
// Created by null on 20-2-1.
//
#include <string>
#include <iostream>
#include <tuple>
#include <cstring>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <ff_api.h>
#include <engine/reactor/flow.hh>

rte_ring* primary_ring= nullptr;
rte_mempool* message_pool;
struct init_struct{
    int kq;
    int sd;
    kevent* pkevset;
    kevent events[512];
    init_struct(int k,int s,kevent* p):kq(k),sd(s),pkevset(p){}
};

struct connect_data{
    int fd;
};


int main(int argc,char* argv[]){

    engine::reactor::test1();

    ff_init(argc,argv);

    rte_ring* x[RTE_MAX_LCORE];

    if(rte_eal_process_type() == RTE_PROC_PRIMARY){
        primary_ring=rte_ring_create("PRIMARY_RING",64,rte_socket_id(),0);
        message_pool = rte_mempool_create("PRIMARY_RING", 1024,
                                          sizeof(connect_data), 0, 0,
                                          NULL, NULL, NULL, NULL,
                                          rte_socket_id(), 0);
    }
    else{
        primary_ring=rte_ring_lookup("PRIMARY_RING");
        message_pool=rte_mempool_lookup("PRIMARY_RING");
    }

    ff_run
            (
                    [](void* arg){
                        auto a= static_cast<init_struct*>(arg);
                        timespec ts;
                        ts.tv_nsec=0;
                        ts.tv_sec=0;
                        uint32_t nevents=ff_kevent(a->kq, nullptr, 0, a->events,512, nullptr);
                        for(int i=0;i<nevents;++i){
                            if(a->events[i].flags&EV_EOF){
                                ff_close(static_cast<int>(a->events[i].ident));
                                continue;
                            }
                            if(a->sd== static_cast<int>(a->events[i].ident)){
                                int count= static_cast<int>(a->events[i].data);
                                std::cout<<count<<std::endl;
                                do{

                                    int fd=ff_accept(a->sd, nullptr, nullptr);

                                    EV_SET(a->pkevset, fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
                                    ff_kevent(a->kq,a->pkevset,1, nullptr,0, nullptr);
                                    std::cout<<"000000000000"<<std::endl;

                                }
                                while(--count);
                                continue;
                            }
                            if(EVFILT_READ==a->events[i].filter){
                                char buf[256];
                                std::cout<<"111111111111"<<std::endl;
                                size_t len=ff_read(static_cast<int>(a->events[i].ident),buf, sizeof(buf));
                                std::cout<<"222222222222"<<std::endl;
                                ff_write(static_cast<int>(a->events[i].ident),buf,len);
                                std::cout<<"recv in core"<<rte_lcore_id()<<std::endl;
                                continue;
                            }
                            std::cout<<"unknown event:"<< a->events[i].flags<<std::endl;
                        }

                        return 0;
                    },
                    []()->void*{

                        int kq=ff_kqueue();
                        kevent* pkevset=new kevent();

                        int sd = ff_socket(AF_INET, SOCK_STREAM, 0);
                        sockaddr_in my_addr;
                        memset(&my_addr, sizeof(my_addr),0);
                        my_addr.sin_family = AF_INET;
                        my_addr.sin_port = htons(80);
                        my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                        ff_bind(sd, (struct linux_sockaddr *)&my_addr, sizeof(my_addr));
                        //int reusepoer=1;
                        //std::cout<<ff_setsockopt(sd,SOL_SOCKET,SO_REUSEPORT,&reusepoer, sizeof(int))<<std::endl;
                        ff_listen(sd, 512);

                        EV_SET(pkevset, sd, EVFILT_READ, EV_ADD, 0, 512, nullptr);
                        ff_kevent(kq, pkevset, 1, nullptr, 0, nullptr);
                        return new init_struct(kq,sd,pkevset);

                    }()
            );

    return 0;
}

