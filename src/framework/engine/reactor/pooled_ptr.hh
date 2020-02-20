//
// Created by anyone on 20-2-17.
//

#ifndef PROJECT_POOLED_PTR_HH
#define PROJECT_POOLED_PTR_HH


#include <memory>
#include <boost/noncopyable.hpp>

namespace engine{
    namespace utils{
        template <typename _P_>
        struct pooled_ptr:boost::noncopyable{
            _P_* _p;
            template <typename ..._ARG_>
            void
            create(_ARG_&&... args){
                _p=new _P_(std::forward<_ARG_>(args)...);
            }
            void
            destory(){

            }
        };
    }
}
#endif //PROJECT_POOLED_PTR_HH
