//
// Created by root on 2020/5/22.
//

#ifndef PUMP_EE_HH
#define PUMP_EE_HH
#include <variant>
#include <reactor/flow.hh>
#include <f-stack/ff_api.h>
#include <f-stack/ff_api.h>
#include <f-stack/ff_config.h>
#include <dpdk/include/rte_ring.h>
#include <dpdk/include/rte_mempool.h>
#include <dpdk/include/rte_ethdev.h>
#include <common/defer.hh>

#include <dpdk/net/tcp_listener.hh>
#include <dpdk/net/tcp_connector.hh>
#include <dpdk/net/tcp_session.hh>



#include <vector>
//#include <boost/program_options.hpp>
//#include <boost/lexical_cast.hpp>
//#include <boost/property_tree/ptree.hpp>
//#include <boost/property_tree/json_parser.hpp>

using namespace std;

namespace yy{
    auto ii(){}
    void vv(){
        vector<int> a{1,2,3,4,5};
        //std::ranges::split_view(a,[](auto a){return a>3;});
        ii();
        std::variant<int,long> v;
        std::get<long>(v);
    }
}
#endif //PUMP_EE_HH
