//
// Created by root on 2020/3/6.
//

#ifndef PUMP_DPDK_CHANNEL_HH
#define PUMP_DPDK_CHANNEL_HH

#include <list>
#include <common/ncpy_func.hh>
#include <engine/reactor/schedule.hh>

namespace engine::dpdk_channel{

    struct channel_msg{
        int cmd;
        int len;
        char data[];
    };

    template <typename _MSG_>
    struct out_channel{
        rte_ring* r;
        rte_mempool* m;
        auto
        push_msg(_MSG_* msg){
            return reactor::make_imme_flow(msg)
                    .then([this](FLOW_ARG(_MSG_*)&& v){
                        ____forward_flow_monostate_exception(v);
                        auto&& msg=std::get<_MSG_*>(v);
                        void* msg0;
                        rte_mempool_get(m,&msg0);
                        std::memcpy(msg0,msg,sizeof(_MSG_));
                        if(rte_ring_enqueue(r,msg0)<0)
                            throw std::logic_error("cant push msg to channel");
                        else
                            return;
                    });
        }
    };

    template <typename _MSG_>
    struct in_channel{
        rte_ring* r;
        rte_mempool* m;
        std::list<_MSG_*> waiting_msgs;
        std::list<common::ncpy_func<void(FLOW_ARG(_MSG_*)&&)>> waiting_tasks;
        void
        schedule(common::ncpy_func<void(FLOW_ARG(_MSG_*)&&)>&& task){
            waiting_tasks.emplace_back(std::forward<common::ncpy_func<void(FLOW_ARG(_MSG_*)&&)>>(task));
        }
        auto
        push_msg(_MSG_* msg){
            return reactor::make_imme_flow(msg)
                    .then([this](FLOW_ARG(_MSG_*)&& v){
                        ____forward_flow_monostate_exception(v);
                        auto&& msg=std::get<_MSG_*>(v);
                        void* msg0;
                        rte_mempool_get(m,&msg0);
                        std::memcpy(msg0,msg,sizeof(_MSG_));
                        if(rte_ring_enqueue(r,msg0)<0)
                            throw std::logic_error("cant push msg to channel");
                        else
                            return;
                    });
        }
        void
        pull_msg(){
            void* msg0;
            if(rte_ring_dequeue(r,&msg0)<0)
                return;
            auto* msg1= reinterpret_cast<channel_msg*>(new char[sizeof(_MSG_)]);
            std::memcpy(msg1,msg0,sizeof(_MSG_));
            rte_mempool_put(m,msg1);
            if(!waiting_tasks.empty()){
                auto&& v=waiting_tasks.front();
                v(FLOW_ARG(channel_msg*)(msg1));
                waiting_tasks.pop_front();
            }
            else{
                waiting_msgs.push_back(msg1);
            }
        }
        auto
        wait_msg(){
            if(waiting_msgs.empty()){
                return reactor::flow_builder<channel_msg*>::at_schedule
                        (
                                [this](std::shared_ptr<reactor::flow_implent<channel_msg*>> sp_flow){
                                    schedule([sp_flow](FLOW_ARG(channel_msg*)&& v){
                                        sp_flow->trigge(std::forward<FLOW_ARG(channel_msg*)>(v));
                                    });
                                },
                                reactor::_sp_immediate_runner_
                        );
            }
            else{
                channel_msg* r=waiting_msgs.front();
                waiting_tasks.pop_front();
                return reactor::make_imme_flow(std::forward<channel_msg*>(r));
            }
        }
    };

    template <typename _MSG_>
    in_channel<_MSG_> recv_channel;
    template <typename _MSG_>
    out_channel<_MSG_> send_channel[128];
}
#endif //PUMP_DPDK_CHANNEL_HH
