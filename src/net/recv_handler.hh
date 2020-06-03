//
// Created by root on 2020/5/26.
//

#ifndef PUMP_RECV_HANDLER_HH
#define PUMP_RECV_HANDLER_HH
namespace pump::net{
    template <typename _RECV_BUF_>
    struct recv_handler{
        size_t need_len;
        common::ncpy_func<bool()> available;
        common::ncpy_func<void(FLOW_ARG(sp<_RECV_BUF_>)&&)> handle;
    };
}
#endif //PUMP_RECV_HANDLER_HH
