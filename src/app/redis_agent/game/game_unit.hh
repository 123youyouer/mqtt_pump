//
// Created by root on 2020/4/16.
//

#ifndef PUMP_GAME_UNIT_HH
#define PUMP_GAME_UNIT_HH

#include <boost/tuple/tuple.hpp>
#include <boost/hana.hpp>
namespace game{

    class _com_body{
    public:
        int x,y,r;
    };
    class _com_moveable{
    public:
        int speed;
        int face;
    };
    class _com_deadable{
    public:
        int hp;
    };

    template <class ..._COMPONENT_>
    class game_unit{
    private:
        constexpr static auto x=boost::hana::make_tuple(boost::hana::type_c<_COMPONENT_>...);
        std::tuple<_COMPONENT_...> components;
    public:
        template <typename _T_>
        __always_inline _T_&
        get(){
            return std::get<_T_>(components);
        }
        template <typename _T_>
        __always_inline constexpr static auto
        has(){
            return boost::hana::contains(x,boost::hana::type_c<_T_>);
        }
    };

    using wolf=game_unit<_com_deadable,_com_moveable,_com_body>;
    using tree=game_unit<_com_body>;

    void move(_com_moveable& m,_com_body& b){
        b.x=m.speed*10+b.x;
    }
    template <typename _U_>
    void move(_U_& u){
        move(u.template get<_com_moveable>(),u.template get<_com_body>());
    }


    void test(){
        wolf w;
        tree t;

        if constexpr (wolf::has<_com_moveable>())
            move(w);
        if constexpr (tree::has<_com_moveable>())
            move(t);
    }
}
#endif //PUMP_GAME_UNIT_HH
