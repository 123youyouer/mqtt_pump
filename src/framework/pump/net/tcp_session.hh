//
// Created by root on 2020/5/21.
//

#ifndef PUMP_TCP_SESSION_HH
#define PUMP_TCP_SESSION_HH
namespace pump::net{
    template <typename T>
    concept HAS_EMPTY = requires(T a){
        {a.empty()}->std::same_as<bool> ;

    };
    template <typename T>
    concept HAS_AAA = requires(T a){
        {a.aaa()}->std::same_as<std::string>;

    };


    template <typename T>
    concept HAS_E=HAS_EMPTY<T>||HAS_AAA<T>;

    template <HAS_E T> auto
    wait_packet(T& t,int ms=0,size_t len=0){
        if(!t.empty())
            std::cout<<t.aaa1()<<std::endl;
        if constexpr (std::is_same_v<decltype(t.empty()),bool>)
            return 10;
        else
            return 11;
    }
}
#endif //PUMP_TCP_SESSION_HH
