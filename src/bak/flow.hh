//
// Created by null on 19-11-20.
//

#ifndef PUMP_FLOW_HH
#define PUMP_FLOW_HH

#include <iostream>
#include <memory>
#include <utility>
#include <tuple>
#include <functional>
#include <boost/hana.hpp>
#include "utils/noncopyable_function.hh"
#include "flow_state.hh"
#include "schedule.hh"
#include <utils/can_inherit.hh>
#include <utils/any.hh>

namespace reactor{
    template <typename _V>
    class flow_builder;

    template <typename _V>
    struct flow_builder_type{
        using type = _V;
    };

    template <typename _V>
    struct flow_builder_type<flow_builder<_V>>{
        using type=_V;
    };

    template <typename T, bool is_trivial_class>
    class flow_value{};

    template <typename _V>
    class flow_value<_V,false>{
    protected:
        any<_V> _v;
    public:
        explicit flow_value()= default;
        virtual ~flow_value()= default;
        [[gnu::always_inline]][[gnu::hot]]
        void set_value(_V&& arg){
            new (&_v.value) _V(std::forward<_V>(arg));
        }
        [[gnu::always_inline]][[gnu::hot]]
        _V& get_value(){
            return _v.value;
        }
    };

    template <>
    class flow_value<void,false>{
    public:
        [[gnu::always_inline]][[gnu::hot]]
        void set_value(){
        }
        [[gnu::always_inline]][[gnu::hot]]
        void get_value(){
        }
    };

    template <typename _V>
    class flow_value<_V,true>{
    public:
        explicit flow_value()= default;
        [[gnu::always_inline]][[gnu::hot]]
        void set_value(_V&& v) {
            new (this) _V(std::move(v));
        }
        [[gnu::always_inline]][[gnu::hot]]
        _V& get_value() {
            return *this;
        }
    };

    template <typename _V>
    class flow_type_def{
    public:
        using task_func_type=utils::noncopyable_function<void(_V&&)>;
        using args_type=_V&&;
    };

    template <>
    class flow_type_def<void>{
    public:
        using task_func_type=utils::noncopyable_function<void()>;
        using args_type=void;
    };
    template <typename _Arg>
    class flow_base:public flow_value<_Arg,can_inherit<_Arg>>,boost::noncopyable{
    protected:
        std::unique_ptr<typename flow_type_def<_Arg>::task_func_type> _unique_ptr_task;
    public:
        std::shared_ptr<int> runing_cpu;
    public:
        virtual ~flow_base()= default;
        explicit flow_base()= default;
        template <typename _F>
        void set_task(_F&& f) {
            _unique_ptr_task.reset(new (typename flow_type_def<_Arg>::task_func_type)(std::forward<_F>(f)));
        }
    };

    template<typename _Arg>
    class flow:public flow_base<_Arg>{
    public:
        virtual ~flow()= default;
        explicit flow()= default;
        explicit flow(_Arg&& arg){
            flow_base<_Arg>::set_value(std::forward<_Arg>(arg));
        }
        [[gnu::always_inline]][[gnu::hot]]
        void active(){
            if(flow_base<_Arg>::_unique_ptr_task)
                (*flow_base<_Arg>::_unique_ptr_task)(std::forward<_Arg>(this->get_value()));

        }
        [[gnu::always_inline]][[gnu::hot]]
        void active(_Arg&& a){
            this->set_value(std::forward<_Arg>(a));
            active();
        }
    };
    template <>
    class flow<void>:public flow_base<void>{
    public:
        explicit flow()= default;
        [[gnu::always_inline]][[gnu::hot]]
        void active(){
            if(flow_base<void>::_unique_ptr_task)
                (*flow_base<void>::_unique_ptr_task)();
        }
    };

    template<typename _V>
    class flow_builder_base:boost::noncopyable{
    public:
        std::shared_ptr<flow<_V>> _shared_ptr_flow;
        std::unique_ptr<utils::noncopyable_function<void(int)>> _unique_ptr_first_submit;
        explicit flow_builder_base()= default;
        explicit flow_builder_base
                (std::unique_ptr<utils::noncopyable_function<void(int)>>& unique_ptr_submit,std::shared_ptr<flow<_V>>& shared_ptr_flow)
                :_shared_ptr_flow(shared_ptr_flow)
                ,_unique_ptr_first_submit(unique_ptr_submit.release()){
        }
        [[gnu::always_inline]][[gnu::hot]]
        void submit(const unsigned int& to_cpu){
            (*flow_builder_base<_V>::_unique_ptr_first_submit)(to_cpu);
        }
        template <typename _F>
        [[gnu::always_inline]][[gnu::hot]]
        auto then(_F&& f);
    };
    template <typename _V>
    class flow_builder:public flow_builder_base<_V>{
    public:
        explicit flow_builder(std::unique_ptr<utils::noncopyable_function<void(int)>>& unique_ptr_submit,std::shared_ptr<flow<_V>>& shared_ptr_flow)
                :flow_builder_base<_V>(unique_ptr_submit,shared_ptr_flow){
        }
        explicit flow_builder(_V&& v){
            flow_builder_base<_V>::_shared_ptr_flow.reset(new flow<_V>(std::forward<_V>(v)));
            flow_builder_base<_V>::_unique_ptr_first_submit.reset(
                    new utils::noncopyable_function<void(int)>(
                            [_shared_ptr_flow=flow_builder_base<_V>::_shared_ptr_flow](int to_cpu){
                                _shared_ptr_flow->runing_cpu=std::make_shared<int>(int(to_cpu));
                                return (_shared_ptr_flow)->active();
                            })
            );
        }
        explicit flow_builder()= delete;
        flow_builder<_V>& set_value(_V&& v){
            this->_shared_ptr_flow->set_value(std::forward<_V>(v));
            return *this;
        }
    };

    template <>
    class flow_builder<void>:public flow_builder_base<void>{
    public:
        explicit flow_builder(std::unique_ptr<utils::noncopyable_function<void(int)>>& unique_ptr_submit,std::shared_ptr<flow<void>>& shared_ptr_flow)
                :flow_builder_base<void>(unique_ptr_submit,shared_ptr_flow){
        }
    };

    template<typename _1_FLOW_TYPE,typename _2_FLOW_TYPE,typename _FUNC_TYPE>
    constexpr
    auto task_function_builder=[](std::shared_ptr<flow<_1_FLOW_TYPE>>&& fw1,std::shared_ptr<flow<_2_FLOW_TYPE>>&& fw2,_FUNC_TYPE&& f){
        using namespace boost::hana;
        if constexpr (equal(type_c<_1_FLOW_TYPE>,type_c<void>)){
            using _R=std::result_of_t<_FUNC_TYPE()>;
            if constexpr (equal(type_c<_2_FLOW_TYPE>,type_c<void>)){
                return [_fw1=fw1,_fw2=fw2,f]()mutable{
                    if(!_fw2->runing_cpu)
                        _fw2->runing_cpu=_fw1->runing_cpu;
                    internal::schdule(
                            utils::noncopyable_function<void()>(
                                    [_fw1=_fw1,_fw2=_fw2,_f=utils::noncopyable_function<_R()>(std::forward<_FUNC_TYPE>(f))](){
                                        _f();
                                        _fw2->active();
                                    })
                            ,*(_fw1->runing_cpu)
                            ,*(_fw2->runing_cpu)
                    );

                };
            }
            else if constexpr(equal(type_c<_R>,type_c<flow_builder<_2_FLOW_TYPE>>)){
                return [_fw1=fw1,_fw2=fw2,f]()mutable{
                    if(!_fw2->runing_cpu)
                        _fw2->runing_cpu=_fw1->runing_cpu;
                    internal::schdule(
                            utils::noncopyable_function<void()>
                                    ([_fw1=_fw1,_fw2=_fw2,_f=utils::noncopyable_function<_R()>(std::forward<_FUNC_TYPE>(f))](){
                                        _f()
                                                .then([_fw2=_fw2](){
                                                    _fw2->active();
                                                })
                                                .submit(*(_fw2->runing_cpu));
                                    })
                            ,*(_fw1->runing_cpu)
                            ,*(_fw2->runing_cpu)
                    );
                };
            }
            else{
                return [_fw1=fw1,_fw2=fw2,f]()mutable{
                    if(!_fw2->runing_cpu)
                        _fw2->runing_cpu=_fw1->runing_cpu;
                    internal::schdule(
                            utils::noncopyable_function<void()>
                                    ([_fw1=_fw1,_fw2=_fw2,_f=utils::noncopyable_function<_R()>(std::forward<_FUNC_TYPE>(f))](){
                                        _fw2->active(std::forward<_R>(_f()));
                                    })
                            ,*(_fw1->runing_cpu)
                            ,*(_fw2->runing_cpu)
                    );
                };
            }
        }
        else{
            using _R=std::result_of_t<_FUNC_TYPE(_1_FLOW_TYPE&&)>;
            if constexpr (equal(type_c<_2_FLOW_TYPE>,type_c<void>)){
                return [_fw1=fw1,_fw2=fw2,f](_1_FLOW_TYPE&& _arg)mutable{
                    if(!_fw2->runing_cpu)
                        _fw2->runing_cpu=_fw1->runing_cpu;
                    internal::schdule(
                            utils::noncopyable_function<void()>
                                    ([_fw1=_fw1,_fw2=_fw2,_f=utils::noncopyable_function<_R(_1_FLOW_TYPE&&)>(std::forward<_FUNC_TYPE>(f))](){
                                        _f(std::forward<_1_FLOW_TYPE>(_fw1->get_value()));
                                        _fw2->active();
                                    })
                            ,*(_fw1->runing_cpu)
                            ,*(_fw2->runing_cpu)
                    );
                };
            }
            else if constexpr(equal(type_c<_R>,type_c<flow_builder<_2_FLOW_TYPE>>)){
                return [_fw1=fw1,_fw2=fw2,f](_1_FLOW_TYPE&& _arg)mutable{
                    if(!_fw2->runing_cpu)
                        _fw2->runing_cpu=_fw1->runing_cpu;
                    internal::schdule(
                            utils::noncopyable_function<void()>
                                    ([_fw1=_fw1,_fw2=_fw2,_f=utils::noncopyable_function<_R(_1_FLOW_TYPE&&)>(std::forward<_FUNC_TYPE>(f))](){
                                        _f(std::forward<_1_FLOW_TYPE>(_fw1->get_value()))
                                                .then([_fw2=_fw2](_2_FLOW_TYPE&& _arg){
                                                    _fw2->active(std::forward<_2_FLOW_TYPE>(_arg));
                                                })
                                                .submit(*(_fw2->runing_cpu));
                                    })
                            ,*(_fw1->runing_cpu)
                            ,*(_fw2->runing_cpu)
                    );
                };
            }
            else{
                return [_fw1=fw1,_fw2=fw2,f](_1_FLOW_TYPE&& _arg)mutable{
                    if(!_fw2->runing_cpu)
                        _fw2->runing_cpu=_fw1->runing_cpu;
                    internal::schdule(
                            utils::noncopyable_function<void()>
                                    ([_fw1=_fw1,_fw2=_fw2,_f=utils::noncopyable_function<_R(_1_FLOW_TYPE&&)>(std::forward<_FUNC_TYPE>(f))](){
                                        _fw2->active(std::forward<_R>(_f(std::forward<_1_FLOW_TYPE>(_fw1->get_value()))));
                                    })
                            ,*(_fw1->runing_cpu)
                            ,*(_fw2->runing_cpu)
                    );
                };
            }

        }
    };

    template <typename _V>
    template <typename _F>
    inline
    auto flow_builder_base<_V>::then(_F&& f){
        using namespace boost::hana;
        if constexpr (equal(type_c<_V>,type_c<void>)){
            using _R=std::result_of_t<_F()>;
            using _TRUE_RES_TYPE_=typename flow_builder_type<_R>::type;
            std::shared_ptr<flow<_TRUE_RES_TYPE_>> shared_ptr_next_flow(new flow<_TRUE_RES_TYPE_>());
            (*flow_builder_base<_V>::_shared_ptr_flow).set_task(
                    task_function_builder<_V,_TRUE_RES_TYPE_,_F>(
                            std::forward<std::shared_ptr<flow<_V>>>(flow_builder_base<_V>::_shared_ptr_flow),
                            std::forward<std::shared_ptr<flow<_TRUE_RES_TYPE_>>>(shared_ptr_next_flow),
                            std::forward<_F>(f))
            );

            return flow_builder<_TRUE_RES_TYPE_> (flow_builder_base<_V>::_unique_ptr_first_submit,shared_ptr_next_flow);
        }
        else{
            using _R=std::result_of_t<_F(_V&&)>;
            using _TRUE_RES_TYPE_=typename flow_builder_type<_R>::type;
            std::shared_ptr<flow<_TRUE_RES_TYPE_>> shared_ptr_next_flow(new flow<_TRUE_RES_TYPE_>());
            (*flow_builder_base<_V>::_shared_ptr_flow).set_task(
                    task_function_builder<_V,_TRUE_RES_TYPE_,_F>(
                            std::forward<std::shared_ptr<flow<_V>>>(flow_builder_base<_V>::_shared_ptr_flow),
                            std::forward<std::shared_ptr<flow<_TRUE_RES_TYPE_>>>(shared_ptr_next_flow),
                            std::forward<_F>(f))
            );
            return flow_builder<_TRUE_RES_TYPE_> (flow_builder_base<_V>::_unique_ptr_first_submit,shared_ptr_next_flow);
        }
    }
}
#endif //PUMP_FLOW_HH
