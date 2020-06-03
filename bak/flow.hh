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
#include <engine/reactor/schedule.hh>

namespace engine{
    namespace reactor{

        namespace bha=boost::hana;

        template <typename ..._ARG_>
        class flow_implent;
        template <typename ..._ARG_>
        class flow_builder;

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
                return bha::type_c<std::result_of_t<_FUNC_(_arg_list_type_&&)>>;
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
            using _act_func_type_=common::ncpy_func<void(_arg_list_type_&&)>;
        protected:
            _act_func_type_ _act_func_;
            std::shared_ptr<flow_runner> _runner;
        public:
            template <typename _FUNC_, typename _NEXT_>
            void
            assemble_act_func_(_FUNC_ &&f, _NEXT_ &&n);
            void
            assemble_sch_func_(std::shared_ptr<flow_runner>& r){
                _runner=r;
            }
            flow_implent()= default;
        public:
            ALWAYS_INLINE void
            active(_arg_list_type_&& _a){
                if(_act_func_)
                    _runner->schedule(([_f=std::move(_act_func_),_a=std::move(_a)](FLOW_ARG()&& state)mutable{
                        _f(std::move(_a));
                    }));
            }
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
                                    _submit_at_([_impl](FLOW_ARG(_ARG_...)&& in){
                                        _impl->active(std::forward<FLOW_ARG(_ARG_...)>(in));
                                    });
                                },
                                (_impl)
                        );
            };
            static constexpr auto
            at_schedule(std::shared_ptr<flow_runner>& _schedule_at_){
                return at_schedule
                        (
                                [_sch=_schedule_at_](auto&& f)mutable{
                                    _sch->schedule(std::forward<decltype(f)>(f));
                                },
                                _schedule_at_
                        );
            };
        };

        template <typename ..._ARG_>
        template <typename _FUNC_, typename _NEXT_>
        void
        flow_implent<_ARG_...>::assemble_act_func_(_FUNC_ &&f, _NEXT_ &&n){
            using cb=flow_type_compute<_ARG_...>;
            using re=typename decltype(cb::template compute_this_func_res_type<_FUNC_>())::type;

            _act_func_=[_func=std::move(f),_next=std::forward<_NEXT_>(n)](_arg_list_type_&& args)mutable{
                if constexpr (std::is_same_v<void,re>){
                    try{
                        _func(std::move(args));
                        _next->active(FLOW_ARG()(std::monostate()));
                    }
                    catch(...){
                        _next->active(FLOW_ARG()(std::current_exception()));
                    }

                }
                else if constexpr (bha::equal(
                        bha::type_c<re>,
                        bha::type_c<typename remove_flow_implent_shell<re>::_T_>
                )){
                    try{
                        _next->active(FLOW_ARG(re)(_func(std::move(args))));
                    }
                    catch (...){
                        _next->active(FLOW_ARG(re)(std::current_exception()));
                    }
                }
                else{
                    try{
                        _func(std::forward<_arg_list_type_>(args))
                                .then([_next](FLOW_ARG(typename remove_flow_implent_shell<re>::_T_)&& v){
                                    _next->active(std::forward<FLOW_ARG(typename remove_flow_implent_shell<re>::_T_)>(v));
                                })
                                .submit();
                    }
                    catch(...){
                        _next->active(FLOW_ARG(typename remove_flow_implent_shell<re>::_T_)(std::current_exception()));
                    }

                }

            };
        }

        auto
        make_task_flow(){
            return flow_builder<>::at_schedule(_sp_global_task_center_);
        }
        auto
        make_imme_flow(){
            return flow_builder<>::at_schedule(_sp_immediate_runner_);
        }
    }
}
#endif //PROJECT_FLOW_HH
