//
// Created by root on 2020/5/24.
//

#ifndef PUMP_WAIT_PACKET_HH
#define PUMP_WAIT_PACKET_HH
#include <common/concept.hh>
#include <reactor/flow.hh>
#include <net/recv_handler.hh>
namespace pump::net{
    template<typename T>
    concept _RECV_BUF_CEPT_=requires (T t){
        {t.size()}->pump::common::same_as<size_t>;
    }&& requires (T t){
        {t.empty()}->common::same_as<bool>;
    };

    template <typename T1,typename T2>
    concept _RECV_SCHEDULER_CEPT_=requires (T1 t1){
        {t1.schedule(int(),common::ncpy_func<bool()>(),common::ncpy_func<void(sp<T2>)>())};
    };

    template <typename T>
    concept _TIME_SCHEDULE_CEPT_=requires (T t){
        {t.schedule(int(),common::ncpy_func<void()>())};
    };

    template <typename _RECV_BUF_,typename _RECV_SCHE,typename _TIME_SCHE_>
    requires _RECV_BUF_CEPT_<_RECV_BUF_> && _RECV_SCHEDULER_CEPT_<_RECV_SCHE,_RECV_BUF_> && _TIME_SCHEDULE_CEPT_<_TIME_SCHE_>
    auto
    wait_packet(sp<_RECV_BUF_> recv_buf,sp<_RECV_SCHE> recv_sche,sp<_TIME_SCHE_> time_sche,int ms=0,size_t len=0){
        if(recv_buf->empty() || recv_buf->size()<len){
            using f_type=common::ncpy_func<void(FLOW_ARG(std::variant<int,common::ringbuffer*>))>;
            return reactor::flow_builder<std::variant<int,sp<_RECV_BUF_>>>::at_schedule
                    (
                            [recv_sche,ms,time_sche,len](sp<reactor::flow_implent<std::variant<int,sp<_RECV_BUF_>>>> sp_flow)->void{
                                recv_sche->schedule(
                                        len,
                                        [sp_flow](){ return !sp_flow->called();},
                                        [sp_flow](FLOW_ARG(sp<_RECV_BUF_>)&& v){
                                            switch (v.index()){
                                                case 0:
                                                    sp_flow->trigge(FLOW_ARG(std::variant<int,sp<_RECV_BUF_>>)(std::get<0>(v)));
                                                    return;
                                                case 1:
                                                    sp_flow->trigge(FLOW_ARG(std::variant<int,sp<_RECV_BUF_>>)(std::get<1>(v)));
                                                    return;
                                                default:
                                                    sp_flow->trigge(FLOW_ARG(std::variant<int,sp<_RECV_BUF_>>)(
                                                            std::variant<int,sp<_RECV_BUF_>>(std::get<2>(v))));
                                                    return;
                                            }
                                        });
                                if(ms<=0)
                                    return;
                                time_sche->schdule(
                                        ms,
                                        [ms,sp_flow](){
                                            sp_flow->trigge(FLOW_ARG(std::variant<int,sp<_RECV_BUF_>>)(
                                                    std::variant<int,sp<_RECV_BUF_>>(ms)));
                                        });
                            },
                            reactor::_sp_immediate_runner_
                    );
        }
        else{
            return reactor::make_imme_flow(std::variant<int,sp<_RECV_BUF_>>(recv_buf));
        }
    }
}
#endif //PUMP_WAIT_PACKET_HH
