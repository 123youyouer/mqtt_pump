//
// Created by null on 19-12-27.
//

#ifndef PROJECT_FLOW_HH
#define PROJECT_FLOW_HH
#include <boost/hana.hpp>
#include <hw/cpu.hh>
#include <utils/noncopyable_function.hh>
#include <reactor/schedule.hh>
#include <reactor/flow_state.hh>
namespace reactor{
    template <typename _A,typename _R,bool startor>
    class flow;

    template <typename _T>
    class flow_builder;

    template <typename T>
    class _compute_flow_res_type_{
    public:
        using _T_ =T;
    };

    template <typename T>
    class _compute_flow_res_type_<std::optional<T>>{
    public:
        using _T_ =T;
    };

    template <typename _A,typename _R,bool startor>
    class _compute_flow_res_type_<flow<_A,_R,startor>*>{
    public:
        using _T_ =_R;
    };
    template <typename _V>
    class _compute_flow_res_type_<flow_builder<_V>>{
    public:
        using _T_ =_V;
    };

    template <typename _V>
    class next_able;

    template <typename _V>
    class prev_able{
    public:
        next_able<_V>* _next;
        virtual void submit()=0;
        prev_able():_next(nullptr){}
    };

    template <typename _V>
    class next_able:public schedule_able<_V>{
    public:
        prev_able<_V>* _prev;
        bool immediate;
        bool is_exception_handler;
        virtual void active(_V&& v)=0;
        virtual void handle_exception(std::exception_ptr ex)=0;
        next_able():immediate(false),is_exception_handler(false),_prev(nullptr){}
    };


    template <>
    class next_able<void>:public schedule_able<void>{
    public:
        bool immediate;
        prev_able<void >* _prev;
        virtual void active()=0;
        virtual void handle_exception(std::exception_ptr ex)=0;
        next_able():immediate(false),_prev(nullptr){}
    };

    template <typename _A,typename _R,bool startor=false>
    class flow_base
            :public prev_able<typename _compute_flow_res_type_<_R>::_T_>
                    ,public next_able<typename _compute_flow_res_type_<_A>::_T_>
                    ,public boost::noncopyable{
    protected:
        utils::noncopyable_function<std::optional<_R>(std::exception_ptr)> _exception_func;
    public:
        using exception_handler_res_type=utils::noncopyable_function<std::optional<_R>(std::exception_ptr)>;
        void handle_exception(std::exception_ptr ex)override{
            using namespace boost;
            if(_exception_func){
                std::optional<_R> res= _exception_func(std::forward<std::exception_ptr>(ex));
                if(!res)
                    return;
                if(!this->_next)
                    return;
                if constexpr(hana::equal(hana::type_c<_R>,hana::type_c<typename _compute_flow_res_type_<_R>::_T_>)){
                    this->_next->active(std::forward<_R>(res.value()));
                    delete this;
                }
                else{
                    res.value()._flow->_next=this->_next;
                    delete this;
                    res.value().submit();
                }
            }
            else if(this->_next){
                this->_next->handle_exception(std::forward<std::exception_ptr>(ex));
                delete this;
            }
            else{
                try {
                    if(ex)
                        std::rethrow_exception(ex);
                }
                catch (const std::exception& e){
                    std::cout<<e.what()<<std::endl;
                }
                catch (...){
                    std::cout<<"cant catch exception in flow::when_exception"<<std::endl;
                }
            }
        }
        virtual ~flow_base()= default;
    };

    template <typename _A,bool startor>
    class flow_base<_A,void,startor>
            :public prev_able<typename _compute_flow_res_type_<void>::_T_>
                    ,public next_able<typename _compute_flow_res_type_<_A>::_T_>
                    ,public boost::noncopyable{
    protected:
        utils::noncopyable_function<bool(std::exception_ptr)> _exception_func;
    public:
        using exception_handler_res_type=utils::noncopyable_function<bool(std::exception_ptr)>;
        void handle_exception(std::exception_ptr ex)override{
            using namespace boost;
            if(_exception_func){
                bool res= _exception_func(std::forward<std::exception_ptr>(ex));
                if(!res)
                    return;
                if(!this->_next)
                    return;
                this->_next->active();
            }
            else if(this->_next){
                this->_next->handle_exception(std::forward<std::exception_ptr>(ex));
                delete this;
            }
            else{
                try {
                    if(ex)
                        std::rethrow_exception(ex);
                }
                catch (const std::exception& e){
                    std::cout<<e.what()<<std::endl;
                }
            }
        }
        virtual ~flow_base()= default;
    };


    template <typename _A,typename _R,bool startor=false>
    class
    flow:public flow_base<_A,_R,startor>{
    private:
        auto
        build_active_function(_A&& a){
            using namespace boost;
            if constexpr (hana::equal(hana::type_c<_R>,hana::type_c<void>)){
                _func(std::forward<_A>(a));
                return [next=this->_next,this]()mutable{
                    delete(this);
                    if(next)
                        next->active();
                };
            }
            else{
                if constexpr(hana::equal(hana::type_c<_R>,hana::type_c<typename _compute_flow_res_type_<_R>::_T_>)){
                    return [r=std::forward<_R>(_func(std::forward<_A>(a))),next=this->_next,this]()mutable{
                        delete(this);
                        if(next)
                            next->active(std::forward<_R>(r));
                    };
                }
                else{
                    return [r=std::forward<_R>(_func(std::forward<_A>(a))),next=this->_next,this]()mutable{
                        delete(this);
                        r._flow->_next=next;
                        r.submit();
                    };
                }
            }
        }
    public:
        explicit
        flow()= default;
        explicit
        flow(utils::noncopyable_function<_R(_A&&)>&& f)
                :_func(std::forward<utils::noncopyable_function<_R(_A&&)>>(f))
                ,flow_base<_A,_R,startor>()
        {}
        explicit
        flow(utils::noncopyable_function<_R(_A&&)>&& f,typename next_able<_A>::_running_context_type_& c)
                :_func(std::forward<utils::noncopyable_function<_R(_A&&)>>(f))
                ,flow_base<_A,_R,startor>()
        {
            this->set_schedule_context(c);
        }
        utils::noncopyable_function<_R(_A&&)> _func;

        [[gnu::always_inline]][[gnu::hot]]
        void
        submit()override{
            if constexpr (!startor){
                return this->_prev->submit();
            }
            else{
                if constexpr (!boost::hana::equal(
                        boost::hana::type_c<_running_context_type_none_>,
                        boost::hana::type_c<typename schedule_able<_A>::_running_context_type_>)){
                    schedule_to<typename schedule_able<_A>::_running_context_type_>::apply(this->_rc_,[this](_A&& a)mutable{
                        this->active(std::forward<_A>(a));
                    });
                }
            }
        }

        [[gnu::always_inline]][[gnu::hot]]
        void
        active(_A&& a) override {
            try {
                if constexpr (!startor){
                    if(this->_next)
                        schedule_to<typename schedule_able<_A>::_running_context_type_,typename schedule_able<_R>::_running_context_type_>
                        ::apply(
                                this->_rc_,
                                this->_next->_rc_,
                                build_active_function(std::forward<_A>(a))
                        );
                    else
                        _func(std::forward<_A>(a));
                }
                else{
                    if(this->_next){
                        if constexpr (boost::hana::equal(
                                boost::hana::type_c<typename schedule_able<_R>::_running_context_type_>,
                                boost::hana::type_c<hw::cpu_core>
                        )){
                            if(hw::get_thread_cpu_id()==this->_next->_rc_){
                                build_active_function(std::forward<_A>(a))();
                            }
                            else{
                                schedule_to<typename schedule_able<_R>::_running_context_type_>
                                ::apply(
                                        this->_next->_rc_,
                                        build_active_function(std::forward<_A>(a))
                                );
                            }
                        }
                        else{
                            build_active_function(std::forward<_A>(a))();
                        }
                    }
                    else{
                        _func(std::forward<_A>(a));
                    }
                }
            }
            catch (...){
                this->handle_exception(std::current_exception());
            }


        }
    };

    template <typename _R,bool startor>
    class flow<void,_R,startor>:public flow_base<void,_R,startor>{
    private:
        auto
        build_active_function(){
            using namespace boost;
            if constexpr (hana::equal(hana::type_c<_R>,hana::type_c<void>)){
                _func();
                return [next=this->_next,this]()mutable{
                    delete(this);
                    next->active();
                };
            }
            else{
                if constexpr(hana::equal(hana::type_c<_R>,hana::type_c<typename _compute_flow_res_type_<_R>::_T_>)){
                    return [r=std::forward<_R>(_func()),next=this->_next,this]()mutable{
                        delete(this);
                        if(next)
                            next->active(std::forward<_R>(r));
                    };
                }
                else{
                    return [r=std::forward<_R>(_func()),next=this->_next,this]()mutable{
                        delete(this);
                        r._flow->_next=next;
                        r.submit();
                    };
                }
            }
        }
    public:
        explicit flow()= default;
        explicit flow(utils::noncopyable_function<_R()>&& f)
                :_func(std::forward<utils::noncopyable_function<_R()>>(f))
                ,flow_base<void,_R,startor>() {}
        explicit flow(utils::noncopyable_function<_R()>&& f,const typename next_able<void>::_running_context_type_ & c)
                :_func(std::forward<utils::noncopyable_function<_R()>>(f))
                ,flow_base<void,_R,startor>()
        {
            this->set_schedule_context(c);
        }
        utils::noncopyable_function<_R()> _func;

        [[gnu::always_inline]][[gnu::hot]]
        void
        submit() override {
            if constexpr (!startor)
                return this->_prev->submit();
            schedule_to<typename schedule_able<void>::_running_context_type_>::apply(this->_rc_,[this]()mutable{
                this->active();
            });
        }

        [[gnu::always_inline]][[gnu::hot]]
        void
        active() override {
            try{
                if constexpr (!startor){
                    if(this->_next)
                        schedule_to<typename schedule_able<void>::_running_context_type_,typename schedule_able<_R>::_running_context_type_>
                        ::apply(this->_rc_,this->_next->_rc_,build_active_function());
                    else
                        _func();
                }
                else{
                    if(this->_next){
                        if constexpr (boost::hana::equal(
                                boost::hana::type_c<typename schedule_able<_R>::_running_context_type_>,
                                boost::hana::type_c<hw::cpu_core>
                        )){
                            if(hw::get_thread_cpu_id()==this->_next->_rc_){
                                build_active_function()();
                            }
                            else{
                                schedule_to<typename schedule_able<_R>::_running_context_type_>
                                ::apply(
                                        this->_next->_rc_,
                                        build_active_function()
                                );
                            }
                        }
                        else{
                            build_active_function()();
                        }
                    }
                    else{
                        _func();
                    }
                }
            }
            catch (...){
                this->handle_exception(std::current_exception());
            }

        }
    };

    template <typename _A>
    class flow_exception_handler:public flow<_A,_A,false>{
    public:
        explicit flow_exception_handler(typename flow<_A,_A,false>::exception_handler_res_type&& f)
        :flow<_A,_A,false>([](_A&& a){ return std::forward<_A>(a);}){
            this->_exception_func=std::forward<typename flow<_A,_A,false>::exception_handler_res_type&&>(f);
        }
    };
    template <>
    class flow_exception_handler<void>:public flow<void,void,false>{
    public:
        explicit flow_exception_handler(typename flow<void,void,false>::exception_handler_res_type&& f)
        :flow<void,void,false>([](){ return;}){
            this->_exception_func=std::forward<typename flow<void,void,false>::exception_handler_res_type&&>(f);
        }
    };


    template <typename _V>
    class flow_builder{
    public:
        prev_able<_V>* _flow;
    public:
        explicit
        flow_builder(prev_able<_V>* _f):_flow(_f){}
        template <typename _F,typename _R=std::result_of_t<_F(typename _compute_flow_res_type_<_V>::_T_&&)>>
        flow_builder<typename _compute_flow_res_type_<_R>::_T_> then(_F &&f){
            return then(std::forward<_F>(f), next_able<_V>::_default_running_context_);
        }
        template <typename _F,typename _R=std::result_of_t<_F(typename _compute_flow_res_type_<_V>::_T_&&)>>
        flow_builder<typename _compute_flow_res_type_<_R>::_T_>
        then(_F &&f, const typename next_able<_V>::_running_context_type_ &c){
            auto res=new flow<_V,_R,false>(std::forward<_F>(f));
            res->set_schedule_context(c);
            res->_prev=_flow;
            _flow->_next=res;
            return flow_builder<typename _compute_flow_res_type_<_R>::_T_>(res);
        }
        void
        submit(){
            _flow->submit();
        }
        template <typename _F,typename _R=std::result_of_t<_F(std::exception_ptr)>>
        flow_builder<_V>
        when_exception(_F &&f){
            using namespace boost;
            static_assert(hana::equal(hana::type_c<_R>,hana::type_c<std::optional<_V>>));
            auto res=new flow_exception_handler<_V>(std::forward<_F>(f));
            res->set_schedule_context(next_able<_V>::_default_running_context_);
            res->_prev=_flow;
            _flow->_next=res;
            return flow_builder<_V>(res);
        }

    };
    template <>
    class flow_builder<void>{
    public:
        prev_able<void>* _flow;
    public:
        explicit
        flow_builder(prev_able<void>* _f):_flow(_f){}
        template <typename _F,typename _R=std::result_of_t<_F()>>
        flow_builder<typename _compute_flow_res_type_<_R>::_T_> then(_F &&f){
            return then(std::forward<_F>(f), next_able<void>::_default_running_context_);
        }
        template <typename _F,typename _R=std::result_of_t<_F()>>
        flow_builder<typename _compute_flow_res_type_<_R>::_T_>
        then(_F &&f, const typename next_able<void>::_running_context_type_ &c){
            auto res=new flow<void,_R,false>(std::forward<_F>(f));
            res->set_schedule_context(c);
            res->_prev=_flow;
            _flow->_next=res;
            return flow_builder<typename _compute_flow_res_type_<_R>::_T_>(res);
        }

        void submit(){
            _flow->submit();
        }
        template <typename _F,typename _R=std::result_of_t<_F(std::exception_ptr)>>
        flow_builder<void>
        when_exception(_F &&f){
            auto res=new flow_exception_handler<void>(std::forward<_F>(f));
            res->set_schedule_context(next_able<void>::_default_running_context_);
            res->_prev=_flow;
            _flow->_next=res;
            return flow_builder<void>(res);
        }
    };

    template <typename _A,typename _F>
    auto
    make_flow(_F &&f, typename next_able<_A>::_running_context_type_ &c = next_able<_A>::_default_running_context_){
        return flow_builder(new flow<_A,std::result_of_t<_F(_A&&)>,true>(std::forward<_F>(f),c));
    }
    template <typename _F>
    auto
    make_flow(_F &&f,
                   const typename next_able<void>::_running_context_type_ &c = next_able<void>::_default_running_context_){
        return flow_builder(new flow<void ,std::result_of_t<_F()>,true>(std::forward<_F>(f),c));
    }
}
#endif //PROJECT_FLOW_HH
