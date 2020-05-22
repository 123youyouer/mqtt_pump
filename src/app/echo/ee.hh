//
// Created by root on 2020/5/22.
//

#ifndef PUMP_EE_HH
#define PUMP_EE_HH
#include <variant>
namespace yy{
    auto ii(){}
    auto vv(){
        ii();
        std::variant<int,long> v;
        std::get<long>(v);
    }
}
#endif //PUMP_EE_HH
