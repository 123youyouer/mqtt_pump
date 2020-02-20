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
#include <boost/noncopyable.hpp>
#include <common/noncopyable_function.hh>
#include <common/apply.hh>
#include <boost/hana.hpp>
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


        template<typename _FUNC_,typename ..._ARG_>
        class flow_type_compute:boost::noncopyable{
        public:
            flow_type_compute()= delete;
            flow_type_compute(flow_type_compute&&)= delete;
        public:
            static constexpr auto
            compute_this_func_res_type(){
                if constexpr (bha::equal(
                        bha::type_c<bha::tuple<_ARG_...>>,
                        bha::type_c<bha::tuple<void>>)){
                    return bha::type_c<std::result_of_t<_FUNC_()>>;
                }
                else{
                    return bha::type_c<std::result_of_t<_FUNC_(_ARG_&&...)>>;
                }
            }

            static constexpr auto
            compute_next_flow_implent_type(){
                using res_type=typename remove_flow_implent_shell<typename decltype(compute_this_func_res_type())::type>::_T_ ;
                if constexpr (std::is_same<void,res_type>::value){
                    return bha::type_c<flow_implent<>>;
                }
                else{
                    return bha::type_c<flow_implent<res_type>>;
                }
            }
            static constexpr auto
            compute_next_flow_builder_type(){
                using res_type=typename remove_flow_implent_shell<typename decltype(compute_this_func_res_type())::type>::_T_ ;
                if constexpr (std::is_same<void,res_type>::value){
                    return bha::type_c<flow_builder<>>;
                }
                else{
                    return bha::type_c<flow_builder<res_type>>;
                }
            }
        };

        template <typename ..._ARG_>
        class flow_builder;

        template <typename ..._ARG_>
        class flow_implent:boost::noncopyable{
            using _act_func_type_=common::noncopyable_function<void(_ARG_&&...)>;
            using _sch_func_type_=common::noncopyable_function<void(_act_func_type_&&,_ARG_&&...)>;
        protected:
            _act_func_type_ _act_func_;
            _sch_func_type_ _sch_func_;
        public:
            template <typename _FUNC_, typename _NEXT_>
            void
            assemble_act_func_(_FUNC_ &&f, _NEXT_ &&n);
            template <typename _SCHD_>
            void
            assemble_sch_func_(_SCHD_* s){
                _sch_func_=[s](_act_func_type_&& _f,_ARG_&&..._a)mutable{
                    (*s)([_f=std::forward<_act_func_type_>(_f),_a=std::make_tuple(std::forward<_ARG_>(_a)...)]()mutable{
                        common::apply(std::forward<_act_func_type_>(_f),std::forward<std::tuple<_ARG_...>>(_a));
                    });
                };
            }

            flow_implent()= default;
        public:
            void
            active(_ARG_&&... args){
                if(_act_func_)
                    _sch_func_(std::forward<_act_func_type_>(_act_func_),std::forward<_ARG_>(args)...);
            }
            friend class flow_builder<_ARG_...>;
        };

        template <typename ..._IN_>
        class schdule_center{
        public:
            virtual void operator()(common::noncopyable_function<void(_IN_&& ..._in)>&& f)=0;
        };

        class test_schdule final:public schdule_center<>{
        public:
            std::list<common::noncopyable_function<void()>> l;
            void operator()(common::noncopyable_function<void()>&& f)final{
                l.push_back(std::forward<common::noncopyable_function<void()>>(f));
            }
        };


        template <typename ..._ARG_>
        class flow_builder:boost::noncopyable{
        public:
            struct flow_builder_data{
                common::noncopyable_function<void()> _begin_func_;
                std::shared_ptr<flow_implent<_ARG_...>> _this_impl_;
                schdule_center<>* _psch;
                explicit
                flow_builder_data(
                        common::noncopyable_function<void()>&& _bf,
                        schdule_center<>* sch,
                        std::shared_ptr<flow_implent<_ARG_...>>& imp
                ){
                    _begin_func_=std::forward<common::noncopyable_function<void()>>(_bf);
                    _psch=sch;
                    _this_impl_=imp;
                }
                explicit
                flow_builder_data()= default;
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
                    common::noncopyable_function<void()>&& _bf,
                    schdule_center<>* sch,
                    std::shared_ptr<flow_implent<_ARG_...>>& imp){
                _data=std::shared_ptr<flow_builder_data>(new flow_builder_data(std::forward<common::noncopyable_function<void()>>(_bf),sch,imp));
            }

            explicit
            flow_builder(
                    schdule_center<>* sch,
                    std::shared_ptr<flow_implent<_ARG_...>>& imp)
                    :_data(new flow_builder_data()){
                _data->_psch=sch;
                _data->_this_impl_=imp;
            }
            explicit
            flow_builder(
                    schdule_center<>* sch,
                    std::shared_ptr<flow_implent<_ARG_...>>&& imp)
                    :_data(new flow_builder_data()){
                _data->_psch=sch;
                _data->_this_impl_=imp;
            }
            template <typename _FUNC_>
            auto
            then(_FUNC_&& f){
                using cp=flow_type_compute<_FUNC_,_ARG_...>;
                using fi=typename decltype(cp::compute_next_flow_implent_type())::type;
                using fb=typename decltype(cp::compute_next_flow_builder_type())::type;

                auto _next=std::make_shared<fi>();
                _data->_this_impl_->assemble_act_func_(
                        std::forward<_FUNC_>(f),
                        _next);
                _data->_this_impl_->assemble_sch_func_(_data->_psch);
                return fb(std::forward<common::noncopyable_function<void()>>(this->_data->_begin_func_),_data->_psch,_next);
            }

            void submit(){
                _data->_begin_func_();
            }
            template<typename ..._IN_>
            auto
            to_schdule(schdule_center<_IN_...>* sch){
                this->_data->_psch=sch;
                return std::forward<flow_builder<_ARG_...>>(*this);
            }

            template<typename ..._IN_>
            static auto
            at_schdule(schdule_center<_IN_...>* sch){
                auto x=flow_builder(sch,std::shared_ptr<flow_implent<_ARG_...>>(new flow_implent<_ARG_...>()));
                x._data->_begin_func_=[sch,data=x._data]()mutable{
                    (*sch)([data](_IN_&& ...in){
                        data->_this_impl_->active(std::forward<_IN_>(in)...);
                    });
                };
                return x;
            }
        };

        template <typename ..._ARG_>
        template <typename _FUNC_, typename _NEXT_>
        void
        flow_implent<_ARG_...>::assemble_act_func_(_FUNC_ &&f, _NEXT_ &&n){
            using cb=flow_type_compute<_FUNC_,_ARG_...>;
            using re=typename decltype(cb::compute_this_func_res_type())::type;

            _act_func_=[_func=std::forward<_FUNC_>(f),_next=std::forward<_NEXT_>(n)](_ARG_&&... args){
                if constexpr (std::is_same_v<void,re>){
                    _func(std::forward<_ARG_>(args)...);
                    _next->active();
                }
                else if constexpr (bha::equal(
                        bha::type_c<re>,
                        bha::type_c<typename remove_flow_implent_shell<re>::_T_>
                )){
                    _next->active(_func(std::forward<_ARG_>(args)...));
                }
                else{
                    _func(std::forward<_ARG_>(args)...)
                            .then([_next](typename remove_flow_implent_shell<re>::_T_&& v){
                                _next->active(std::forward<typename remove_flow_implent_shell<re>::_T_>(v));
                            })
                            .submit();
                }

            };
        }

        void test12(test_schdule* p){
            flow_builder<>::at_schdule(p)
                    .then([p](){
                        std::cout<<10<<std::endl;
                        return 10;
                    })
                    .then([p](int&& i){
                        return flow_builder<>::at_schdule(p)
                                .then([](){
                                    return 100;
                                })
                                .then([p](int&& i){
                                    return flow_builder<>::at_schdule(p)
                                            .then([i=std::forward<int>(i)](){
                                                return i*10;
                                            });
                                });
                    })
                    .then([](int&& i){
                        std::cout<<i<<std::endl;
                        return ++i;
                    })

                    .then([](int&& i){
                        std::cout<<i<<std::endl;
                        return ++i;
                    })
                    .to_schdule(new test_schdule())
                    .then([](int&& i){
                        std::cout<<i<<std::endl;
                        return ++i;
                    })
                    .then([](int&& i){
                        std::cout<<i<<std::endl;
                        return ++i;
                    })
                    .submit();
        }

        void test1(){
            test_schdule* p=new test_schdule();
            test12(p);

            while(true){
                if(!p->l.empty()){
                    (*p->l.begin())();
                    p->l.pop_front();
                }
                else{
                    sleep(1);
                }

            }

        }
    }
}
#endif //PROJECT_FLOW_HH
