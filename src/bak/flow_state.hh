//
// Created by null on 19-12-1.
//

#ifndef PROJECT_FLOW_STATE_HH
#define PROJECT_FLOW_STATE_HH

#include <utility>
#include <tuple>
#include <boost/noncopyable.hpp>
namespace reactor{
    class flow_state : public boost::noncopyable{
    public:
        enum class state : uintptr_t {
            invalid = 0,
            future = 1,
            result = 2,
            exception_min = 3,  // or anything greater
        };
        union any{
            state st;
            std::exception_ptr ex;
            any() { st = state::future; }
            any(state s) { st = s; }
            any(any&& x) {
                if (x.st < state::exception_min) {
                    st = x.st;
                } else {
                    new (&ex) std::exception_ptr(x.take_exception());
                }
                x.st = state::invalid;
            }
            any(std::exception_ptr e) {
                set_exception(std::move(e));
            }
            void set_exception(std::exception_ptr e) {
                new (&ex) std::exception_ptr(std::move(e));
            }
            std::exception_ptr take_exception(){
                std::exception_ptr ret(std::move(ex));
                ex.~exception_ptr();
                st = state::invalid;
                return ret;
            }
        } _any;

    public:
        bool available() const noexcept {
            return _any.st == state::result || _any.st >= state::exception_min;
        }
        bool failed() const noexcept {
            return __builtin_expect(_any.st >= state::exception_min, false);
        }
        void set_exception(std::exception_ptr ex) noexcept {
            _any.set_exception(std::move(ex));
        }
        std::exception_ptr get_exception() && noexcept {
            return _any.take_exception();
        }

    };
}
#endif //PROJECT_FLOW_STATE_HH
