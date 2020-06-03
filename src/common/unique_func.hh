//
// Created by root on 20-2-27.
//

#ifndef PROJECT_UNIQUE_FUNC_HH
#define PROJECT_UNIQUE_FUNC_HH

#include <functional>
#include <memory>
#include <boost/noncopyable.hpp>

namespace pump::common{
    template <typename T>
    class unique_func{};
    template <typename Ret, typename... Args>
    class unique_func<Ret(Args...)>:boost::noncopyable{
        std::unique_ptr<std::function<Ret(Args...)>> _imp_;
    public:
        unique_func()=delete;
        unique_func(const unique_func&& f)noexcept{
            this->_imp=f._imp_;
        }
        unique_func(unique_func&& f)noexcept{
            this->_imp=f._imp_;
        }
    };
}

#endif //PROJECT_UNIQUE_FUNC_HH
