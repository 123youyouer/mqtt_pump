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
                    return bha::type_c<std::variant<std::monostate,std::exception_ptr>>;
                }
                else if constexpr (sizeof...(_ARG_)==0){
                    return bha::type_c<std::variant<std::monostate,std::exception_ptr>>;
                }
                else if constexpr (sizeof...(_ARG_)==1){
                    return bha::type_c<std::variant<std::monostate,std::exception_ptr,_ARG_...>>;
                }
                else{
                    return bha::type_c<std::variant<std::monostate,std::exception_ptr,std::tuple<_ARG_...>>>;
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
                using res_type=typename remove_flow_implent_shell<typename decltype(compute_this_func_res_type<_FUNC_>())::type>::_T_ ;
                if constexpr (std::is_same<void,res_type>::value){
                    return bha::type_c<flow_implent<>>;
                }
                else{
                    return bha::type_c<flow_implent<res_type>>;
                }
            }
            template <typename _FUNC_>
            static constexpr auto
            compute_next_flow_builder_type(){
                using res_type=typename remove_flow_implent_shell<typename decltype(compute_this_func_res_type<_FUNC_>())::type>::_T_ ;
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
            using _arg_list_type_=typename decltype(flow_type_compute<_ARG_...>::compute_arg_type())::type;
            using _act_func_type_=common::noncopyable_function<void(_arg_list_type_&&)>;
            using _sch_func_type_=common::noncopyable_function<void(_act_func_type_&&,_arg_list_type_&&)>;
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
                _sch_func_=[s](_act_func_type_&& _f,_arg_list_type_&& _a)mutable{
                    (*s)([_f=std::forward<_act_func_type_>(_f),_a=std::forward<_arg_list_type_>(_a)]()mutable{
                        _f(std::forward<_arg_list_type_>(_a));
                    });
                };
            }

            flow_implent()= default;
        public:
            void
            active(_arg_list_type_&& a){
                if(_act_func_)
                    _sch_func_(std::forward<_act_func_type_>(_act_func_),std::forward<_arg_list_type_>(a));
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
                using cp=flow_type_compute<_ARG_...>;
                using fi=typename decltype(cp::template compute_next_flow_implent_type<_FUNC_>())::type;
                using fb=typename decltype(cp::template compute_next_flow_builder_type<_FUNC_>())::type;

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
                        if constexpr (bha::equal(
                                bha::type_c<std::tuple<_IN_...>>,
                                bha::type_c<std::tuple<void>>
                                )){
                            data->_this_impl_->active(std::variant<std::monostate,std::exception_ptr>());
                        }
                        else if constexpr (bha::equal(
                                bha::type_c<std::tuple<_IN_...>>,
                                bha::type_c<std::tuple<>>
                        )){

                            data->_this_impl_->active(std::variant<std::monostate,std::exception_ptr>());
                        }
                        else{
                            data->_this_impl_->active(
                                    std::variant<std::monostate,std::exception_ptr,std::tuple<_IN_...>>(
                                            std::make_tuple(std::forward<_IN_>(in)...)
                                    )
                            );
                        }

                    });
                };
                return x;
            }
        };

        template <typename ..._ARG_>
        template <typename _FUNC_, typename _NEXT_>
        void
        flow_implent<_ARG_...>::assemble_act_func_(_FUNC_ &&f, _NEXT_ &&n){
            using cb=flow_type_compute<_ARG_...>;
            using re=typename decltype(cb::template compute_this_func_res_type<_FUNC_>())::type;

            _act_func_=[_func=std::forward<_FUNC_>(f),_next=std::forward<_NEXT_>(n)](_arg_list_type_&& args){
                if constexpr (std::is_same_v<void,re>){
                    std::variant<std::monostate,std::exception_ptr> res;
                    try{
                        _func(std::forward<_arg_list_type_>(args));
                        res.template emplace<std::monostate>();
                    }
                    catch(...){
                        res.template emplace<std::exception_ptr>(std::current_exception());
                    }
                    _next->active(std::forward<std::variant<std::monostate,std::exception_ptr>>(res));
                }
                else if constexpr (bha::equal(
                        bha::type_c<re>,
                        bha::type_c<typename remove_flow_implent_shell<re>::_T_>
                )){
                    std::variant<std::monostate,std::exception_ptr,std::tuple<re>> res;
                    try{
                        if constexpr (std::is_same_v<std::tuple<_ARG_...>,std::tuple<>>){
                            res.template emplace<std::tuple<re>>(std::make_tuple(_func(std::variant<std::monostate,std::exception_ptr>())));
                        }
                        else{
                            res.template emplace<std::tuple<re>>(std::make_tuple(_func(std::forward<_arg_list_type_>(args))));
                        }

                    }
                    catch (...){
                        res.template emplace<std::exception_ptr>(std::current_exception());
                    }
                    _next->active(std::forward<std::variant<std::monostate,std::exception_ptr,std::tuple<re>>>(res));
                }
                else{
                    std::variant<std::monostate,std::exception_ptr,std::tuple<typename remove_flow_implent_shell<re>::_T_>> res;
                    try{
                        _func(std::forward<_arg_list_type_>(args))
                                .then([_next](std::variant<std::monostate,std::exception_ptr,std::tuple<typename remove_flow_implent_shell<re>::_T_>>&& v){
                                    _next->active(
                                            std::forward<
                                                    std::variant<std::monostate,std::exception_ptr,std::tuple<typename remove_flow_implent_shell<re>::_T_>>
                                            >(v));
                                })
                                .submit();
                    }
                    catch(...){
                        _next->active(std::forward<
                                std::variant<
                                        std::monostate,std::exception_ptr,std::tuple<
                                                typename remove_flow_implent_shell<re>::_T_
                                        >
                                >
                        >(res));
                    }

                }

            };
        }

        void test12(test_schdule* p){
            flow_builder<>::at_schdule(p)
                    .then([p](std::variant<std::monostate,std::exception_ptr>&& v){
                        std::cout<<10<<std::endl;
                        throw std::logic_error("yeeee");
                        return 10;
                    })
                    .then([p](std::variant<std::monostate,std::exception_ptr,std::tuple<int>>&& v){
                        switch (v.index()){
                            case 0:
                                return 98;
                            case 1:
                                try {
                                    std::rethrow_exception(std::get<1>(v));
                                }
                                catch (const std::exception& e){
                                    std::cout<<e.what()<<std::endl;
                                }
                                catch (...){
                                    std::cout<<"aaa"<<std::endl;
                                }
                                return 99;
                            case 2:
                                std::cout<<"lll"<<std::endl;
                                return std::get<0>(std::get<2>(v))+1;
                        }
                    })
                    .then([p](std::variant<std::monostate,std::exception_ptr,std::tuple<int>>&& v){
                        return flow_builder<>::at_schdule(p)
                                .then([p](std::variant<std::monostate,std::exception_ptr>&& v){
                                    std::cout<<10<<std::endl;
                                    return 10;
                                })
                                .then([p](std::variant<std::monostate,std::exception_ptr,std::tuple<int>>&& v){
                                    switch (v.index()){
                                        case 0:
                                            return 98;
                                        case 1:
                                            try {
                                                std::rethrow_exception(std::get<1>(v));
                                            }
                                            catch (const std::exception& e){
                                                std::cout<<e.what()<<std::endl;
                                            }
                                            catch (...){
                                                std::cout<<"aaa"<<std::endl;
                                            }
                                            return 99;
                                        case 2:
                                            std::cout<<"lll"<<std::endl;
                                            return std::get<0>(std::get<2>(v))+1;
                                    }
                                });
                    })
                    .then([p](std::variant<std::monostate,std::exception_ptr,std::tuple<int>>&& v){
                        switch (v.index()){
                            case 0:
                                return 98;
                            case 1:
                                try {
                                    std::rethrow_exception(std::get<1>(v));
                                }
                                catch (const std::exception& e){
                                    std::cout<<e.what()<<std::endl;
                                }
                                catch (...){
                                    std::cout<<"aaa"<<std::endl;
                                }
                                return 99;
                            case 2:
                                std::cout<<"98989"<<std::endl;
                                return std::get<0>(std::get<2>(v))+1;
                        }
                    })

                    /*
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
                     */

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
