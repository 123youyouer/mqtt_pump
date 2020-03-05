//
// Created by anyone on 20-2-17.
//

#ifndef PROJECT_FLOW_HH
#define PROJECT_FLOW_HH
#include <iostream>
#include <type_traits>
#include <memory>
#include <variant>
#include <list>
#include <any>
#include <boost/hana.hpp>
#include <boost/noncopyable.hpp>
#include <common/ncpy_func.hh>
#include <common/apply.hh>
#include <common/g_define.hh>
#include <engine/reactor/schdule.hh>

namespace engine::reactor{
    namespace bha=boost::hana;
    template <typename ..._ARG_>
    class flow_implent;
    template <typename ..._ARG_>
    class flow_builder;

    constexpr
    static
    auto unpack=[](auto&& ...T)->auto{
        return boost::hana::type_c<typename std::remove_const<typename std::remove_reference<decltype(T)>::type>::type::type...>;
    };

    template <typename ...T>
    class remove_flow_implent_shell{
    public:
        using _T_ =bha::tuple<T...>;
    };
    template <typename T>
    class remove_flow_implent_shell<T>{
    public:
        using _T_ =T;
    };
    template <typename T>
    class remove_flow_implent_shell<flow_implent<T>>{
    public:
        using _T_ =T;
    };
    template <typename T>
    class remove_flow_implent_shell<flow_builder<T>>{
    public:
        using _T_ =T;
    };

    template <>
    class remove_flow_implent_shell<flow_builder<>>{
    public:
        using _T_ =void;
    };


    template<typename ..._ARG_>
    class flow_type_compute:boost::noncopyable{
    public:
        flow_type_compute()= delete;
        flow_type_compute(flow_type_compute&&)= delete;
    public:
        static constexpr auto
        compute_arg_type(){
            if constexpr (bha::equal(
                    bha::type_c<bha::tuple<_ARG_...>>,
                    bha::type_c<bha::tuple<void>>)){
                return bha::type_c<FLOW_ARG()>;
            }
            else if  constexpr (bha::equal(
                    bha::type_c<bha::tuple<_ARG_...>>,
                    bha::type_c<bha::tuple<flow_builder<>>>)){
                return bha::type_c<FLOW_ARG()>;
            }
            else if  constexpr (bha::equal(
                    bha::type_c<bha::tuple<_ARG_...>>,
                    bha::type_c<bha::tuple<flow_implent<>>>)){
                return bha::type_c<FLOW_ARG()>;
            }
            else if constexpr (sizeof...(_ARG_)==0){
                return bha::type_c<FLOW_ARG()>;
            }
            else if constexpr (sizeof...(_ARG_)==1){
                return bha::type_c<FLOW_ARG(_ARG_...)>;
            }
            else{
                return bha::type_c<FLOW_ARG(std::tuple<_ARG_...>)>;
            }
        }

        template <typename _FUNC_>
        static constexpr auto
        compute_this_func_res_type(){
            using _arg_list_type_=typename decltype(compute_arg_type())::type;
            using _res_type_=std::result_of_t<_FUNC_(_arg_list_type_&&)>;
            return bha::type_c<_res_type_>;
        }

        template <typename _FUNC_>
        static constexpr auto
        compute_next_flow_implent_type(){
            using arg_type=typename decltype(compute_this_func_res_type<_FUNC_>())::type;
            using res_type=typename remove_flow_implent_shell<arg_type>::_T_ ;
            if constexpr (std::is_same_v<void,res_type>){
                return bha::type_c<flow_implent<>>;
            }
            else{
                return bha::type_c<flow_implent<res_type>>;
            }
        }
        template <typename _FUNC_>
        static constexpr auto
        compute_next_flow_builder_type(){
            using arg_type=typename decltype(compute_this_func_res_type<_FUNC_>())::type;
            using res_type=typename remove_flow_implent_shell<arg_type>::_T_ ;
            if constexpr (std::is_same_v<void,res_type>){
                return bha::type_c<flow_builder<>>;
            }
            else{
                return bha::type_c<flow_builder<res_type>>;
            }
        }
    };

    template <typename ..._ARG_>
    class flow_implent:boost::noncopyable{
    public:
        using _arg_list_type_=typename decltype(flow_type_compute<_ARG_...>::compute_arg_type())::type;
        using _act_func_type_=common::ncpy_func<void(_arg_list_type_&)>;
    protected:
        struct _inner_{
            bool called;
            _act_func_type_ _act_func_;
            std::shared_ptr<flow_runner> _runner;
            _arg_list_type_ _value_;
            _inner_():called(false){}
        };
        std::shared_ptr<_inner_> inner_data;
    public:
        template <typename _FUNC_, typename _NEXT_>
        void
        assemble_act_func_(_FUNC_ &&f, std::shared_ptr<_NEXT_> n);
        void
        assemble_sch_func_(std::shared_ptr<flow_runner>& r){
            inner_data->_runner=r;
        }
        flow_implent():inner_data(new _inner_()){};
        flow_implent(flow_implent&& f)noexcept{
            inner_data=f.inner_data;
            //std::swap(inner_data,f.inner_data);
        }
    private:
        ALWAYS_INLINE void
        active_impl(){
            inner_data->_runner->schedule([flow=std::forward<flow_implent<_ARG_...>>(*this)](FLOW_ARG()&& v)mutable{
                flow.inner_data->_act_func_(flow.inner_data->_value_);
            });
        }
    public:
        ALWAYS_INLINE bool
        called(){ return inner_data->called;}
        template <typename _A_>
        ALWAYS_INLINE void
        active(_A_&& arg){
            if(inner_data->called)
                return;
            inner_data->called= true;
            if(!inner_data->_act_func_)
                return;
            inner_data->_value_.template emplace<_A_>(std::forward<_A_>(arg));
            active_impl();
        }
        ALWAYS_INLINE void
        active(){
            if(inner_data->called)
                return;
            inner_data->called= true;
            if(!inner_data->_act_func_)
                return;
            inner_data->_value_.template emplace<std::monostate>(std::monostate());
            active_impl();
        }
        ALWAYS_INLINE void
        active(std::exception_ptr _e){
            if(inner_data->called)
                return;
            inner_data->called= true;
            if(!inner_data->_act_func_)
                return;
            inner_data->_value_.template emplace<std::exception_ptr>(std::forward<std::exception_ptr>(_e));
            active_impl();
        }
        ALWAYS_INLINE void
        trigge(_arg_list_type_&& arg){
            if(inner_data->called)
                return;
            inner_data->called= true;
            if(!inner_data->_act_func_)
                return;
            inner_data->_value_.swap(arg);
            active_impl();
        }
        /*
        ALWAYS_INLINE void
        trigge(std::exception_ptr _e){
            if(inner_data->called)
                return;
            inner_data->called= true;
            if(!inner_data->_act_func_)
                return;
            inner_data->_value_.template emplace<std::exception_ptr>(std::forward<std::exception_ptr>(_e));
            active_impl();
        }
         */
    };

    template <typename ..._ARG_>
    class flow_builder:boost::noncopyable{
        using _sub_func_t_=common::ncpy_func<void()>;
        using _imp_flow_t_=std::shared_ptr<flow_implent<_ARG_...>>;
    public:
        struct flow_builder_data{
            std::shared_ptr<flow_runner> _runner_;
            _sub_func_t_ _sub_func_;
            _imp_flow_t_ _imp_flow_;

            explicit
            flow_builder_data(
                    std::shared_ptr<flow_runner>& _run,
                    _sub_func_t_&& _sub,
                    _imp_flow_t_& _imp
            )
                    :_runner_(_run)
                    ,_sub_func_(std::forward<_sub_func_t_>(_sub))
                    ,_imp_flow_(std::forward<_imp_flow_t_>(_imp)){
            }
            explicit
            flow_builder_data()= delete;
        };
        std::shared_ptr<flow_builder_data> _data;
    public:
        flow_builder(flow_builder&& o)noexcept{
            _data=o._data;
        }
        flow_builder(const flow_builder&& o)noexcept{
            _data=o._data;
        }
        explicit
        flow_builder(
                std::shared_ptr<flow_runner>& _run,
                _sub_func_t_&& _sub,
                _imp_flow_t_& _imp){
            _data=std::make_shared<flow_builder_data>(
                    _run,
                    std::forward<_sub_func_t_>(_sub),
                    (_imp)
            );
        }
        template <typename _FUNC_>
        typename decltype(flow_type_compute<_ARG_...>::template compute_next_flow_builder_type<_FUNC_>())::type
        then(_FUNC_&& f){
            using cp=flow_type_compute<_ARG_...>;
            using fi=typename decltype(cp::template compute_next_flow_implent_type<_FUNC_>())::type;
            using fb=typename decltype(cp::template compute_next_flow_builder_type<_FUNC_>())::type;

            auto _next_flow=std::make_shared<fi>();

            _data->_imp_flow_->assemble_act_func_(std::forward<_FUNC_>(f),_next_flow);
            _data->_imp_flow_->assemble_sch_func_(_data->_runner_);

            return fb(
                    _data->_runner_,
                    std::forward<_sub_func_t_>(_data->_sub_func_),
                    _next_flow
            );
        }

        void submit(){
            _data->_sub_func_();
        }
        flow_builder<_ARG_...>
        to_schedule(std::shared_ptr<flow_runner>& r){
            _data->_runner_=r;
            return std::forward<flow_builder<_ARG_...>>(*this);
        }
        template <typename _SUBMIT_AT_>
        static auto
        at_schedule(_SUBMIT_AT_&& _submit_at_,std::shared_ptr<flow_runner>& _schedule_at_){
            auto _impl=std::make_shared<flow_implent<_ARG_...>>();
            return flow_builder<_ARG_...>
                    (
                            _schedule_at_,
                            [_submit_at_=std::forward<_SUBMIT_AT_>(_submit_at_),_impl]()mutable{
                                _submit_at_(_impl);
                            },
                            (_impl)
                    );
        }
        /*
        template <typename _SUBMIT_AT_>
        static auto
        at_schedule(_SUBMIT_AT_&& _submit_at_,std::shared_ptr<flow_runner>& _schedule_at_){
            auto _impl=std::make_shared<flow_implent<_ARG_...>>();
            return flow_builder<_ARG_...>
                    (
                            _schedule_at_,
                            [_submit_at_=std::forward<_SUBMIT_AT_>(_submit_at_),_impl]()mutable{
                                _submit_at_([_impl](FLOW_ARG(_ARG_...)&& a){
                                    _impl->trigge(std::forward<FLOW_ARG(_ARG_...)>(a));
                                });
                            },
                            (_impl)
                    );
        };
        */
        static constexpr auto
        at_schedule(std::shared_ptr<flow_runner>& _schedule_at_){
            return at_schedule
                    (
                            [_sch=_schedule_at_](std::shared_ptr<flow_implent<_ARG_...>> f)mutable{
                                _sch->schedule([f](FLOW_ARG(_ARG_...)&& a){
                                    f->trigge(std::forward<FLOW_ARG(_ARG_...)>(a));
                                });
                            },
                            _schedule_at_
                    );
        };
    };


    template <typename ..._ARG_>
    template <typename _FUNC_, typename _NEXT_>
    void
    flow_implent<_ARG_...>::assemble_act_func_(_FUNC_ &&f, std::shared_ptr<_NEXT_> n){
        using cb=flow_type_compute<_ARG_...>;
        using re=typename decltype(cb::template compute_this_func_res_type<_FUNC_>())::type;
        inner_data->_act_func_=[_func=std::forward<_FUNC_>(f),_next=n](_arg_list_type_& args)mutable{
            if constexpr (std::is_same_v<void,re>){
                try{
                    _func(std::forward<_arg_list_type_>(args));
                    _next->active();
                }
                catch(...){
                    _next->active(std::current_exception());
                }

            }
            else if constexpr (bha::equal(
                    bha::type_c<re>,
                    bha::type_c<flow_builder<>>)){
                try{
                    _func(std::forward<_arg_list_type_>(args))
                            .then([_next](FLOW_ARG()&& v){
                                _next->trigge(std::forward<FLOW_ARG()>(v));
                            })
                            .submit();
                }
                catch(...){
                    _next->active(std::current_exception());
                }
            }
            else if constexpr (bha::equal(
                    bha::type_c<re>,
                    bha::type_c<typename remove_flow_implent_shell<re>::_T_>
            )){
                try{
                    _next->active(_func(std::forward<_arg_list_type_>(args)));
                }
                catch (...){
                    _next->active(std::current_exception());
                }
            }
            else{
                try{
                    _func(std::forward<_arg_list_type_>(args))
                            .then([_next](FLOW_ARG(typename remove_flow_implent_shell<re>::_T_)&& v){
                                _next->trigge(std::forward<FLOW_ARG(typename remove_flow_implent_shell<re>::_T_)>(v));
                            })
                            .submit();
                }
                catch(...){
                    _next->active(std::current_exception());
                }
            }

        };
    }

    flow_builder<>
    make_task_flow(){
        return flow_builder<>::at_schedule(_sp_global_task_center_);
    }
    flow_builder<>
    make_imme_flow(){
        return flow_builder<>::at_schedule(_sp_immediate_runner_);
    }
}
#endif //PROJECT_FLOW_HH
