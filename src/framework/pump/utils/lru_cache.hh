//
// Created by null on 20-2-3.
//

#ifndef PROJECT_LRU_CACHE_HH
#define PROJECT_LRU_CACHE_HH

#include <memory>
#include <list>
#include <unordered_map>
#include <boost/noncopyable.hpp>
#include <boost/intrusive/list.hpp>
namespace common{

    template <typename _K_,typename _V_>
    struct
    key_value_pair:boost::noncopyable{
        _K_ _k;
        std::unique_ptr<_V_> _v;
        key_value_pair(_K_& k,std::unique_ptr<_V_>& v)
                :_k(k),_v(v.release()){}
        key_value_pair(_K_&& k,std::unique_ptr<_V_>& v)
                :_k(k),_v(v.release()){}
    };

    template <typename _K_,typename _V_>
    class lru_cache final : boost::noncopyable{
    public:
        using node_type=key_value_pair<_K_,_V_>;
        using list_type=std::list<std::shared_ptr<node_type>>;
    private:
        size_t _max_size;
        size_t _elasticity;
        std::unordered_map
                <
                        std::unique_ptr<_K_>,
                        typename list_type::iterator
                > _cache;
        list_type _keys;
    public:
        explicit
        lru_cache(size_t max_size = 64, size_t elasticity = 10)
        : _max_size(max_size), _elasticity(elasticity) {}
        virtual
        ~lru_cache()= default;
        size_t
        size() const {
            return _cache.size();
        }
        bool
        empty(){
            return _cache.empty();
        }
        void
        clear(){
            _keys.clear();
            _cache.clear();
        }
        void
        insert(const _K_& k,std::unique_ptr<_V_>& v){
            _cache.find(k);
        }
        void
        get(const _K_& k){
            _cache.get(k);
        }
    };
}
#endif //PROJECT_LRU_CACHE_HH
