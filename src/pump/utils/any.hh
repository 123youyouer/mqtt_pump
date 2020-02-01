//
// Created by null on 19-12-13.
//

#ifndef PROJECT_ANY_HH
#define PROJECT_ANY_HH
template <typename _V>
union any{
    any(){};
    ~any(){};
    _V value;
};
#endif //PROJECT_ANY_HH
