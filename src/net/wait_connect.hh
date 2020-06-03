//
// Created by root on 2020/5/27.
//

#ifndef PUMP_WAIT_CONNECT_HH
#define PUMP_WAIT_CONNECT_HH
namespace pump::net{

    template <typename LISTENER,typename SESSION>
    concept _LISTENER_CEPT_=requires (LISTENER l){
        {l.schedule(FLOW_ARG(SESSION)())};
    };

    template <typename LISTENER,typename SESSION>
    requires _LISTENER_CEPT_<LISTENER,SESSION>
    auto
    wait_connect(sp<LISTENER> listener){
        return pump::reactor::flow_builder<SESSION>::at_schedule([listener](sp<pump::reactor::flow_implent<SESSION>> f){
            listener->schedule([f](FLOW_ARG(SESSION)&& v){
                f->trigge(std::forward<FLOW_ARG(SESSION)>(v));
            });
        });
    }
}
#endif //PUMP_WAIT_CONNECT_HH
