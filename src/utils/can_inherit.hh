//
// Created by null on 19-12-12.
//

#ifndef PROJECT_CAN_INHERIT_HH
#define PROJECT_CAN_INHERIT_HH

#include <type_traits>

template <typename T>
constexpr bool can_inherit
        =(std::is_trivially_destructible<T>::value &&
          std::is_trivially_constructible<T>::value &&
          std::is_class<T>::value &&
          !std::is_final<T>::value);
#endif //PROJECT_CAN_INHERIT_HH
