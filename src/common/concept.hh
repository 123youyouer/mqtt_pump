//
// Created by root on 2020/5/24.
//

#ifndef PUMP_CONCEPT_HH
#define PUMP_CONCEPT_HH
namespace pump::common{
    template <typename T0,typename T1>
    concept same_as=std::is_same_v<T0,T1>;
}
#endif //PUMP_CONCEPT_HH
