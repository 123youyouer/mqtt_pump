//
// Created by null on 19-12-22.
//

#ifndef PROJECT_FUTURE_HH
#define PROJECT_FUTURE_HH

#include <memory>
#include "utils/noncopyable_function.hh"

namespace reactor{

    template <typename ...T>
    class future{
    public:
        virtual void _noimp_()=0;
    };

    template <typename _V>
    class future<_V>{
    public:
        virtual void active(_V&& v)=0;
    };

    template <typename _A,typename _R>
    class future<_A,_R>:public future<_A>{
    public:
        future<_R>* next;
        utils::noncopyable_function<_R(_A&&)> f;
        void active(_A&& a) override {
            next->active(f(std::forward<_A>(a)));
        }
        template <typename _F,typename _RES_TYPE_=std::result_of_t<_F(_A&&)>>
        future<_R,_RES_TYPE_>& then(_F&& f){
            this->next=new future<_R,_RES_TYPE_>();
            return *(static_cast<future<_R,_RES_TYPE_>*>(this->next));
        }
    };


    void test(){
        future<int,int>()
                .then([](int&& i){
                    return 10;
                })
                .then([](int&& i){
                    return 10;
                });
    }
}
#endif //PROJECT_FUTURE_HH
