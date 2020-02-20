//
// Created by null on 20-1-17.
//

#ifndef PROJECT_FLOW_STATE_HH
#define PROJECT_FLOW_STATE_HH

#include <boost/core/noncopyable.hpp>
#include <exception>

namespace reactor{
    /*
    enum class flow_state_code{
        waiting = 0,
        result = 1,
        exception = 2,
    };

    template <typename T>
    constexpr bool can_inherit=(std::is_trivially_destructible<T>::value && std::is_trivially_constructible<T>::value &&
                                std::is_class<T>::value && !std::is_final<T>::value);

    template <typename T, bool is_trivial_class>
    struct flow_value;

    template <typename T>
    struct flow_value<T, false> {
        union any {
            any() {}
            ~any() {}
            T value;
        } _v;

    public:
        void set_value(T&& v) {
            new (&_v.value) T(std::forward<T>(v));
        }
        T& value() {
            return _v.value;
        }
        const T& value() const {
            return _v.value;
        }
    };

    template <typename T>
    struct flow_value<T, true> : private T {
        void set_value(T&& v) {
            new (this) T(std::forward<T>(v));
        }
        T& value() {
            return *this;
        }
        const T& value() const {
            return *this;
        }
    };

    struct flow_exception{
        union any {
            any() {}
            ~any() {}
            std::exception_ptr ex;
        } _e;
        std::exception_ptr take_exception(){
            std::exception_ptr rtn=std::exception_ptr(std::move(_e.ex));
            _e.ex.~exception_ptr();
            return rtn;
        }
        void set_exception(std::exception_ptr e) {
            new (&_e.ex) std::exception_ptr(std::forward<std::exception_ptr>(e));
        }
    };

    template<typename _V>
    struct flow_state:flow_exception,flow_value<_V,can_inherit<_V>>,boost::noncopyable{
        flow_state_code state_code;
        explicit flow_state(std::exception_ptr ex)noexcept:state_code(flow_state_code::exception){
            flow_exception::set_exception(std::forward<std::exception_ptr>(ex));
            state_code=flow_state_code::exception;
        }
        explicit flow_state(_V&& v)noexcept:state_code(flow_state_code::result){
            flow_value<_V,can_inherit<_V>>::set_value(std::forward<_V>(v));
            state_code=flow_state_code::result;
        }
        flow_state()=delete;
        virtual ~flow_state(){
        };
    };
    template <>
    struct flow_state<void>:flow_exception,boost::noncopyable{
        flow_state_code state_code;
        explicit flow_state(std::exception_ptr ex)noexcept:state_code(flow_state_code::exception){
            flow_exception::set_exception(std::forward<std::exception_ptr>(ex));
            state_code=flow_state_code::exception;
        }
        flow_state(){
            state_code=flow_state_code::result;
        };
        virtual ~flow_state(){
        };
    };
     */
}
#endif //PROJECT_FLOW_STATE_HH
