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
#include <reactor/flow_pool.hh>
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
        bool is_exception_handler;
        virtual void active(_V&& v)=0;
        virtual void handle_exception(std::exception_ptr ex)=0;
        next_able():is_exception_handler(false),_prev(nullptr){}
    };


    template <>
    class next_able<void>:public schedule_able<void>{
    public:
        prev_able<void >* _prev;
        bool is_exception_handler;
        virtual void active()=0;
        virtual void handle_exception(std::exception_ptr ex)=0;
        next_able():_prev(nullptr),is_exception_handler(false){}
    };

    constexpr auto
    has_default_running_context_ = boost::hana::is_valid([](auto t)noexcept -> decltype((void)decltype(t)::type::_default_running_context_){
    });

    template <typename _A,typename _R,bool startor=false>
    class flow_base
            :public prev_able<typename _compute_flow_res_type_<_R>::_T_>
                    ,public next_able<typename _compute_flow_res_type_<_A>::_T_>
                    ,public boost::noncopyable{
    protected:
        utils::noncopyable_function<std::optional<_R>(std::exception_ptr)> _exception_func;
    public:
        using exception_handler_res_type=utils::noncopyable_function<std::optional<_R>(std::exception_ptr)>;
        flow_base()= default;
        void
        reinit(){
            this->is_exception_handler=false;
            this->_prev= nullptr;
            this->_next= nullptr;
            if constexpr (has_default_running_context_(boost::hana::type_c<schedule_able<typename _compute_flow_res_type_<_A>::_T_>>)){
                this->_rc_=schedule_able<typename _compute_flow_res_type_<_A>::_T_>::_default_running_context_;
            }
        }
        void
        handle_exception(std::exception_ptr ex)override{
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
        virtual
        ~flow_base()= default;
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
        flow_base()= default;
        void
        reinit(){
            this->is_exception_handler=false;
            this->_prev= nullptr;
            this->_next= nullptr;
            if constexpr (has_default_running_context_(boost::hana::type_c<schedule_able<typename _compute_flow_res_type_<_A>::_T_>>)){
                this->_rc_=schedule_able<typename _compute_flow_res_type_<_A>::_T_>::_default_running_context_;
            }
        }
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
                catch (...){
                    std::cout<<"uncached exception!!!!!"<<std::endl;
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
                    _local_flow_pool<flow<_A,_R,startor>>().destroy(this);
                    if(next)
                        next->active();
                };
            }
            else{
                if constexpr(hana::equal(hana::type_c<_R>,hana::type_c<typename _compute_flow_res_type_<_R>::_T_>)){
                    return [r=std::forward<_R>(_func(std::forward<_A>(a))),next=this->_next,this]()mutable{
                        _local_flow_pool<flow<_A,_R,startor>>().destroy(this);
                        if(next)
                            next->active(std::forward<_R>(r));
                    };
                }
                else{
                    return [r=std::forward<_R>(_func(std::forward<_A>(a))),next=this->_next,this]()mutable{
                        _local_flow_pool<flow<_A,_R,startor>>().destroy(this);
                        r._flow->_next=next;
                        r.submit();
                    };
                }
            }
        }
    public:
        explicit
        flow()= delete;
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
        auto
        reinit(utils::noncopyable_function<_R(_A&&)>&& f){
            flow_base<_A,_R,startor>::reinit();
            this->_func=std::forward<utils::noncopyable_function<_R(_A&&)>>(f);
            return this;
        }
        auto
        reinit(utils::noncopyable_function<_R(_A&&)>&& f,typename next_able<_A>::_running_context_type_& c){
            flow_base<_A,_R,startor>::reinit();
            this->_func=std::forward<utils::noncopyable_function<_R(_A&&)>>(f);
            this->set_schedule_context(c);
            return this;
        }

        utils::noncopyable_function<_R(_A&&)> _func;

        PUMP_INLINE void
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

        PUMP_INLINE void
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
                    _local_flow_pool<flow<void,_R,startor>>().destroy(this);
                    next->active();
                };
            }
            else{
                if constexpr(hana::equal(hana::type_c<_R>,hana::type_c<typename _compute_flow_res_type_<_R>::_T_>)){
                    return [r=std::forward<_R>(_func()),next=this->_next,this]()mutable{
                        _local_flow_pool<flow<void,_R,startor>>().destroy(this);
                        if(next)
                            next->active(std::forward<_R>(r));
                    };
                }
                else{
                    return [r=std::forward<_R>(_func()),next=this->_next,this]()mutable{
                        _local_flow_pool<flow<void,_R,startor>>().destroy(this);
                        r._flow->_next=next;
                        r.submit();
                    };
                }
            }
        }
    public:
        explicit flow()= delete;
        explicit flow(utils::noncopyable_function<_R()>&& f)
                :_func(std::forward<utils::noncopyable_function<_R()>>(f))
                ,flow_base<void,_R,startor>() {}
        explicit flow(utils::noncopyable_function<_R()>&& f,const typename next_able<void>::_running_context_type_ & c)
                :_func(std::forward<utils::noncopyable_function<_R()>>(f))
                ,flow_base<void,_R,startor>()
        {
            this->set_schedule_context(c);
        }
        auto
        reinit(utils::noncopyable_function<_R()>&& f){
            flow_base<void,_R,startor>::reinit();
            this->_func=std::forward<utils::noncopyable_function<_R()>>(f);
            return this;
        }
        auto
        reinit(utils::noncopyable_function<_R()>&& f,typename next_able<void>::_running_context_type_& c){
            flow_base<void,_R,startor>::reinit();
            this->_func=std::forward<utils::noncopyable_function<_R()>>(f);
            this->set_schedule_context(c);
            return this;
        }

        utils::noncopyable_function<_R()> _func;

        PUMP_INLINE void
        submit() override {
            if constexpr (!startor)
                return this->_prev->submit();
            schedule_to<typename schedule_able<void>::_running_context_type_>::apply(this->_rc_,[this]()mutable{
                this->active();
            });
        }

        PUMP_INLINE void
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
        auto
        reinit(typename flow<_A,_A,false>::exception_handler_res_type&& f){
            flow<_A,_A,false>::reinit([](_A&& a){ return std::forward<_A>(a);});
            this->_exception_func=std::forward<typename flow<_A,_A,false>::exception_handler_res_type&&>(f);
            return this;
        }
    };
    template <>
    class flow_exception_handler<void>:public flow<void,void,false>{
    public:
        explicit
        flow_exception_handler(typename flow<void,void,false>::exception_handler_res_type&& f)
                :flow<void,void,false>([](){ return;}){
            this->_exception_func=std::forward<typename flow<void,void,false>::exception_handler_res_type&&>(f);
        }
        auto
        reinit(typename flow<void,void,false>::exception_handler_res_type&& f){
            flow<void,void,false>::reinit([](){ return ;});
            this->_exception_func=std::forward<typename flow<void,void,false>::exception_handler_res_type&&>(f);
            return this;
        }
    };
    template <typename _A,bool startor>
    class cpu_change_flow{};


    template <typename _A>
    class cpu_change_flow<_A,false>: public flow_base<_A,_A,false>{
    public:
        explicit
        cpu_change_flow(hw::cpu_core cpu):flow_base<_A,_A,false>(){
            this->set_schedule_context(cpu);
        }
        auto
        reinit(hw::cpu_core cpu){
            flow_base<_A,_A,false>::reinit();
            this->set_schedule_context(cpu);
            return this;
        }
        void
        submit()override{
            this->_prev->submit();
        }
        void
        active(_A&& a)override{
            if(!this->_next)
                return;
            this->_next->set_schedule_context(this->_rc_);
            if(this->_rc_==hw::get_thread_cpu_id()){
                this->_next->active(std::forward<_A>(a));
                _local_flow_pool<cpu_change_flow<_A,false>>().destroy(this);
            }
            else{
                schedule_to<hw::cpu_core>::apply(this->_rc_,[a=std::forward<_A>(a),next=this->_next]()mutable{
                    if(next)
                        next->active(std::forward<_A>(a));
                });
                _local_flow_pool<cpu_change_flow<_A,false>>().destroy(this);
            }
        }
    };
    template <typename _A>
    class cpu_change_flow<_A,true>: public flow_base<_A,_A,true>{
        _A _a;
    public:
        explicit
        cpu_change_flow(hw::cpu_core cpu,_A&& a):flow_base<_A,_A,true>(),_a(a){
            this->set_schedule_context(cpu);
        }
        auto
        reinit(hw::cpu_core cpu,_A&& a){
            flow_base<_A,_A,true>::reinit();
            this->_a=a;
            this->set_schedule_context(cpu);
            return this;
        }
        void
        submit()override{
            if(!this->_next)
                return;
            this->_next->set_schedule_context(this->_rc_);
            schedule_to<hw::cpu_core>::apply(this->_rc_,[a=std::forward<_A>(_a),next=this->_next]()mutable{
                if(next)
                    next->active(std::forward<_A>(a));
            });
            _local_flow_pool<cpu_change_flow<_A,true>>().destroy(this);
        }
        void
        active(_A&& a)override{
            if(!this->_next)
                return;
            this->_next->set_schedule_context(this->_rc_);
            if(this->_rc_==hw::get_thread_cpu_id()){
                this->_next->active(std::forward<_A>(a));
                _local_flow_pool<cpu_change_flow<_A,true>>().destroy(this);
            }
            else{
                schedule_to<hw::cpu_core>::apply(this->_rc_,[a=std::forward<_A>(a),next=this->_next]()mutable{
                    if(next)
                        next->active(std::forward<_A>(a));
                });
                _local_flow_pool<cpu_change_flow<_A,true>>().destroy(this);
            }
        }
    };
    template <>
    class cpu_change_flow<void,true>:public flow_base<void,void,true>{
    public:
        explicit
        cpu_change_flow(hw::cpu_core cpu):flow_base<void,void,true>(){
            this->set_schedule_context(cpu);
        }
        auto
        reinit(hw::cpu_core cpu){
            flow_base<void,void,true>::reinit();
            this->set_schedule_context(cpu);
            return this;
        }
        void
        submit()override{
            if(!this->_next)
                return;
            this->_next->set_schedule_context(this->_rc_);
            schedule_to<hw::cpu_core>::apply(this->_rc_,[next=this->_next]()mutable{
                if(next)
                    next->active();
            });

            _local_flow_pool<cpu_change_flow<void,true>>().destroy(this);
        }
        void
        active()override{
            if(!this->_next)
                return;
            this->_next->set_schedule_context(this->_rc_);
            if(this->_rc_==hw::get_thread_cpu_id()){
                this->_next->active();
                _local_flow_pool<cpu_change_flow<void,true>>().destroy(this);
            }
            else{
                schedule_to<hw::cpu_core>::apply(this->_rc_,[next=this->_next]()mutable{
                    if(next)
                        next->active();
                });
                _local_flow_pool<cpu_change_flow<void,true>>().destroy(this);
            }
        }
    };
    template <>
    class cpu_change_flow<void,false>:public flow_base<void,void,false>{
    public:
        explicit
        cpu_change_flow(hw::cpu_core cpu):flow_base<void,void,false>(){
            this->set_schedule_context(cpu);
        }
        auto
        reinit(hw::cpu_core& cpu){
            flow_base<void,void,false>::reinit();
            this->set_schedule_context(cpu);
            return this;
        }
        void
        submit()override{
            this->_prev->submit();
        }
        void
        active()override{
            if(!this->_next)
                return;
            this->_next->set_schedule_context(this->_rc_);
            if(this->_rc_==hw::get_thread_cpu_id()){
                this->_next->active();
                _local_flow_pool<cpu_change_flow<void,false>>().destroy(this);
            }
            else{
                schedule_to<hw::cpu_core>::apply(this->_rc_,[next=this->_next]()mutable{
                    if(next)
                        next->active();
                });
                _local_flow_pool<cpu_change_flow<void,false>>().destroy(this);
            }
        }
    };


    template <typename _V>
    class
    flow_builder{
    private:
        template <typename _F,typename _R=std::result_of_t<_F(typename _compute_flow_res_type_<_V>::_T_&&)>>
        flow_builder<typename _compute_flow_res_type_<_R>::_T_>
        then(_F &&f, const typename next_able<_V>::_running_context_type_ &c){
            auto res=_local_flow_pool<flow<_V,_R,false>>().construct(std::forward<_F>(f));
            res->set_schedule_context(c);
            res->_prev=_flow;
            _flow->_next=res;
            return flow_builder<typename _compute_flow_res_type_<_R>::_T_>(res);
        }
    public:
        prev_able<_V>* _flow;
    public:
        explicit
        flow_builder(prev_able<_V>* _f):_flow(_f){}
        template <typename _F,typename _R=std::result_of_t<_F(typename _compute_flow_res_type_<_V>::_T_&&)>>
        flow_builder<typename _compute_flow_res_type_<_R>::_T_> then(_F &&f){
            return then(std::forward<_F>(f), next_able<_V>::_default_running_context_);
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
            auto res=_local_flow_pool<flow_exception_handler<_V>>().construct(std::forward<_F>(f));
            res->set_schedule_context(next_able<_V>::_default_running_context_);
            res->_prev=_flow;
            _flow->_next=res;
            return flow_builder<_V>(res);
        }
        flow_builder<_V>
        at_cpu(hw::cpu_core cpu){
            cpu_change_flow<_V,false>* res=_local_flow_pool<cpu_change_flow<_V,false>>().construct(cpu);
            res->_prev=_flow;
            _flow->_next=res;
            return flow_builder<_V>(res);
        }

    };
    template <>
    class flow_builder<void>{
    private:
        template <typename _F,typename _R=std::result_of_t<_F()>>
        flow_builder<typename _compute_flow_res_type_<_R>::_T_>
        then(_F &&f, const typename next_able<void>::_running_context_type_ &c){
            auto res=_local_flow_pool<flow<void,_R,false>>().construct(std::forward<_F>(f));
            res->set_schedule_context(c);
            res->_prev=_flow;
            _flow->_next=res;
            return flow_builder<typename _compute_flow_res_type_<_R>::_T_>(res);
        }
    public:
        prev_able<void>* _flow;
    public:
        explicit
        flow_builder(prev_able<void>* _f):_flow(_f){}
        template <typename _F,typename _R=std::result_of_t<_F()>>
        flow_builder<typename _compute_flow_res_type_<_R>::_T_> then(_F &&f){
            return then(std::forward<_F>(f), next_able<void>::_default_running_context_);
        }
        void
        submit(){
            _flow->submit();
        }
        template <typename _F,typename _R=std::result_of_t<_F(std::exception_ptr)>>
        flow_builder<void>
        when_exception(_F &&f){
            auto res=_local_flow_pool<flow_exception_handler<void>>().construct(std::forward<_F>(f));
            res->set_schedule_context(next_able<void>::_default_running_context_);
            res->_prev=_flow;
            _flow->_next=res;
            return flow_builder<void>(res);
        }
        flow_builder<void>
        at_cpu(hw::cpu_core cpu){
            cpu_change_flow<void,false>* res=_local_flow_pool<cpu_change_flow<void,false>>().construct(cpu);
            res->_prev=_flow;
            _flow->_next=res;
            return flow_builder<void>(res);
        }
    };


    template <typename _A>
    auto
    at_cpu(hw::cpu_core cpu,_A&& a){
        return flow_builder(_local_flow_pool<cpu_change_flow<_A,true>>().construct(cpu,std::forward<_A>(a)));
    }
    auto
    at_cpu(hw::cpu_core cpu){
        return flow_builder(_local_flow_pool<cpu_change_flow<void,true>>().construct(cpu));
    }
    template <typename _A,typename _F>
    auto
    at_ctx(_F &&f, typename next_able<_A>::_running_context_type_ &c = next_able<_A>::_default_running_context_){
        return flow_builder(_local_flow_pool<flow<_A,std::result_of_t<_F(_A&&)>,true>>().construct(std::forward<_F>(f),c));
    }
}
#endif //PROJECT_FLOW_HH
