//
// Created by root on 2020/3/6.
//

#ifndef PUMP_DPDK_CHANNEL_HH
#define PUMP_DPDK_CHANNEL_HH

#include <list>
#include <common/ncpy_func.hh>
#include <engine/reactor/schedule.hh>

namespace engine::dpdk_channel{

    constexpr size_t channel_msg_size=1024;

    struct channel_msg{
        int cmd;
        int len;
        char data[];
    };

    struct channel{
        std::list<channel_msg*> waiting_msgs;
        std::list<common::ncpy_func<void(FLOW_ARG(channel_msg*)&&)>> waiting_tasks;
        void
        schedule(common::ncpy_func<void(FLOW_ARG(channel_msg*)&&)>&& task){
            waiting_tasks.emplace_back(std::forward<common::ncpy_func<void(FLOW_ARG(channel_msg*)&&)>>(task));
        }
    };

    channel recv_channel;

    auto
    push_channel_msg(channel_msg* msg,rte_ring* r,rte_mempool* m){
        return reactor::make_imme_flow()
                .then([msg,r,m](FLOW_ARG()&& v){
                    void* msg0;
                    rte_mempool_get(m,&msg0);
                    std::memcpy(msg0,msg,channel_msg_size);
                    if(rte_ring_enqueue(r,msg0)<0)
                        throw std::logic_error("cant push msg to channel");
                    else
                        return;
                });
    }

    auto
    pull_channel_msg(rte_ring* r,rte_mempool* m){
        void* msg0;
        if(rte_ring_dequeue(r,&msg0)<0)
            return;
        auto* msg1= reinterpret_cast<channel_msg*>(new char[channel_msg_size]);
        std::memcpy(msg1,msg0,channel_msg_size);
        rte_mempool_put(m,msg1);
        if(recv_channel.waiting_tasks.empty()){
            recv_channel.waiting_msgs.push_back(msg1);
        }
        else{
            auto&& v=recv_channel.waiting_tasks.front();
            v(FLOW_ARG(channel_msg*)(msg1));
            recv_channel.waiting_tasks.pop_front();
        }
    }

    auto
    wait_channel_msg(){
        if(recv_channel.waiting_msgs.empty()){
            return reactor::flow_builder<channel_msg*>::at_schedule
                    (
                            [](std::shared_ptr<reactor::flow_implent<channel_msg*>> sp_flow){
                                recv_channel.schedule([sp_flow](FLOW_ARG(channel_msg*)&& v){
                                    sp_flow->trigge(std::forward<FLOW_ARG(channel_msg*)>(v));
                                });
                            },
                            reactor::_sp_immediate_runner_
                    );
        }
        else{
            channel_msg* r=recv_channel.waiting_msgs.front();
            recv_channel.waiting_tasks.pop_front();
            return reactor::make_imme_flow(std::forward<channel_msg*>(r));
        }
    }
    void
    test(){
        wait_channel_msg()
                .then([](FLOW_ARG(channel_msg*)&& v){
                    ____forward_flow_monostate_exception(v);
                })
                .submit();
    }
}
#endif //PUMP_DPDK_CHANNEL_HH
