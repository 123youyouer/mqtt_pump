//
// Created by null on 20-2-3.
//

#ifndef PROJECT_LRU_CACHE_HH
#define PROJECT_LRU_CACHE_HH

#include <memory>
#include <unordered_map>
#include <boost/noncopyable.hpp>
#include <boost/intrusive/list.hpp>
namespace utils{
    template <typename _K_,typename _V_>
    struct
    key_value_pair:boost::noncopyable{
        std::unique_ptr<_K_> _k;
        std::unique_ptr<_V_> _v;
        key_value_pair(std::unique_ptr<_K_>& k,std::unique_ptr<_V_>& v):_k(k.release()),_v(v.release()){}
    };

    template <typename _K_,typename _V_>
    class lru_cache final : boost::noncopyable{
    public:
        using node_type=key_value_pair<_K_,_V_>;
        using list_type=boost::intrusive::list<node_type>;
        std::unordered_map<_K_,typename list_type::iterator> a;
    };
}
#endif //PROJECT_LRU_CACHE_HH
