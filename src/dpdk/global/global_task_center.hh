//
// Created by root on 2020/5/25.
//

#ifndef PUMP_FLOBAL_HH
#define PUMP_FLOBAL_HH
#include <reactor/flow.hh>
namespace pump::dpdk::global{
    std::shared_ptr<pump::reactor::flow_runner> _sp_global_task_center_(new pump::reactor::global_task_center());
    pump::reactor::flow_builder<>
    make_task_flow(){
        return pump::reactor::flow_builder<>::at_schedule(_sp_global_task_center_);
    }
}
#endif //PUMP_FLOBAL_HH
