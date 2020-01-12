//
// Created by null on 19-12-27.
//

#ifndef PROJECT_FLOW_HH
#define PROJECT_FLOW_HH
#include <boost/hana.hpp>
#include <hw/cpu.hh>
#include <utils/noncopyable_function.hh>
#include <reactor/schedule.hh>

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
        virtual void active(_V&& v)=0;
        next_able():immediate(false),_prev(nullptr){}
    };

    template <>
    class next_able<void>:public schedule_able<void>{
    public:
        bool immediate;
        prev_able<void >* _prev;
        virtual void active()=0;
        next_able():immediate(false),_prev(nullptr){}
    };

    template <typename _A,typename _R,bool startor=false>
    class flow_base
            :public prev_able<typename _compute_flow_res_type_<_R>::_T_>
            ,public next_able<typename _compute_flow_res_type_<_A>::_T_>
            ,public boost::noncopyable{};


    template <typename _A,typename _R,bool startor=false>
    class flow:public flow_base<_A,_R,startor>{
    private:
        auto build_active_function(_A&& a){
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
                        //r->_next=next;
                        //r->submit();
                        r._flow->_next=next;
                        r.submit();
                    };
                }
            }
        }
    public:
        explicit flow(utils::noncopyable_function<_R(_A&&)>&& f)
                :_func(std::forward<utils::noncopyable_function<_R(_A&&)>>(f))
                ,flow_base<_A,_R,startor>()
        {}
        explicit flow(utils::noncopyable_function<_R(_A&&)>&& f,typename next_able<_A>::_running_context_type_& c)
                :_func(std::forward<utils::noncopyable_function<_R(_A&&)>>(f))
                ,flow_base<_A,_R,startor>()
        {
            this->set_schedule_context(c);
        }
        utils::noncopyable_function<_R(_A&&)> _func;
        void submit()override{
            if constexpr (!startor){
                return this->_prev->submit();
            }
            else{
                schedule_to<typename schedule_able<_A>::_running_context_type_>::apply(this->_rc_,[this](_A&& a)mutable{
                    this->active(std::forward<_A>(a));
                });
            }
        }
        void active(_A&& a) override {
            if constexpr (!startor){
                if(this->_next)
                    schedule_to<typename schedule_able<_A>::_running_context_type_,typename schedule_able<_R>::_running_context_type_>
                            ::apply(this->_rc_,this->_next->_rc_,build_active_function(std::forward<_A>(a)));
                else
                    _func(std::forward<_A>(a));
            }
            else{
                if(this->_next)
                    build_active_function(std::forward<_A>(a))();
                else
                    _func(std::forward<_A>(a));
            }

        }
    };

    template <typename _R,bool startor>
    class flow<void,_R,startor>:public flow_base<void,_R,startor>{
    private:
        auto build_active_function(){
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
                        r->_next=next;
                        r->submit();
                    };
                }
            }
        }
    public:
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
        void submit()override{
            if constexpr (!startor){
                return this->_prev->submit();
            }
            else{
                schedule_to<typename schedule_able<void>::_running_context_type_>::apply(this->_rc_,[this]()mutable{
                    this->active();
                });
            }
        }
        void active() override {
            if constexpr (!startor){
                if(this->_next)
                    schedule_to<typename schedule_able<void>::_running_context_type_,typename schedule_able<_R>::_running_context_type_>
                            ::apply(this->_rc_,this->_next->_rc_,build_active_function());
                else
                    _func();
            }
            else{
                if(this->_next)
                    build_active_function()();
                else
                    _func();
            }
        }
    };


    template <typename _V>
    class flow_builder{
    public:
        prev_able<_V>* _flow;
    public:
        explicit flow_builder(prev_able<_V>* _f):_flow(_f){}
        template <typename _F,typename _R=std::result_of_t<_F(typename _compute_flow_res_type_<_V>::_T_&&)>>
        flow_builder<typename _compute_flow_res_type_<_R>::_T_> and_then(_F&& f){
            return and_then(std::forward<_F>(f),next_able<_V>::_default_running_context_);
        }
        template <typename _F,typename _R=std::result_of_t<_F(typename _compute_flow_res_type_<_V>::_T_&&)>>
        flow_builder<typename _compute_flow_res_type_<_R>::_T_>
                and_then(_F&& f,const typename next_able<_V>::_running_context_type_& c){
            auto res=new flow<_V,_R,false>(std::forward<_F>(f));
            res->set_schedule_context(c);
            res->_prev=_flow;
            _flow->_next=res;
            return flow_builder<typename _compute_flow_res_type_<_R>::_T_>(res);
        }
        void submit(){
            _flow->submit();
        }
    };
    template <>
    class flow_builder<void>{
    public:
        prev_able<void>* _flow;
    public:
        explicit flow_builder(prev_able<void>* _f):_flow(_f){}
        template <typename _F,typename _R=std::result_of_t<_F()>>
        flow_builder<typename _compute_flow_res_type_<_R>::_T_> and_then(_F&& f){
            return and_then(std::forward<_F>(f),next_able<void>::_default_running_context_);
        }
        template <typename _F,typename _R=std::result_of_t<_F()>>
        flow_builder<typename _compute_flow_res_type_<_R>::_T_>
                and_then(_F&& f,const typename next_able<void >::_running_context_type_& c){
            auto res=new flow<void,_R,false>(std::forward<_F>(f));
            res->set_schedule_context(c);
            res->_prev=_flow;
            _flow->_next=res;
            return flow_builder<typename _compute_flow_res_type_<_R>::_T_>(res);
        }
        void submit(){
            _flow->submit();
        }
    };

    template <typename _A,typename _F>
    auto make_flow(_F &&f, typename next_able<_A>::_running_context_type_ &c = next_able<_A>::_default_running_context_){
        return flow_builder(new flow<_A,std::result_of_t<_F(_A&&)>,true>(std::forward<_F>(f),c));
    }
    template <typename _F>
    auto make_flow(_F &&f,
                   const typename next_able<void>::_running_context_type_ &c = next_able<void>::_default_running_context_){
        return flow_builder(new flow<void ,std::result_of_t<_F()>,true>(std::forward<_F>(f),c));
    }
}
#endif //PROJECT_FLOW_HH
