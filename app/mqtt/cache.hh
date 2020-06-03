//
// Created by null on 20-2-4.
//

#ifndef PROJECT_CAHCE_HH
#define PROJECT_CAHCE_HH

#include <boost/noncopyable.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/hana.hpp>
#include <variant>
#include <memory>
#include <common/g_define.hh>

namespace mqtt{
    using namespace boost::hana::literals;
    namespace bi=boost::intrusive;

    template <typename _K_>
    class cache_key{};

    template <>
    class cache_key<const char*>{
        static const char* copy_data(const char* src){
            size_t len=strlen(src)+1;
            char* sz=new char[len];
            std::memcpy(sz,src,len);
            return sz;
        }
        const char* _data;
        std::string_view _view;
        size_t _code;
    public:
        explicit
        cache_key(const char* k,bool copy=true)
                :_data(copy?copy_data(k):k),_view(_data),_code(std::hash<std::string_view>()(_view)){
        }
        virtual
        ~cache_key(){
            delete _data;
        }
        ALWAYS_INLINE [[nodiscard]] size_t
        hash_code()const{
            return _code;
        }
        ALWAYS_INLINE friend bool
        operator==(const cache_key& l,const cache_key& r){
            return (l._code==r._code)&&(r._view==r._view);
        }
    };

    template <typename _K_,typename _V_>
    class cache;

    template <typename _K_,typename _V_>
    class
    cache_entry{
        friend class cache<_K_,_V_>;
    private:
        using uset_hook_type=bi::unordered_set_member_hook<>;
        bi::list_member_hook<> _timer_link;
        uset_hook_type _cache_link;
    public:
        cache_key<_K_> _key;
        std::shared_ptr<_V_> _shared_ptr_data;
        explicit
        cache_entry(_K_ _k,const std::shared_ptr<_V_>& v)noexcept
                :_key(_k),_shared_ptr_data(v){
        }

        ALWAYS_INLINE friend bool
        operator == (const cache_entry<_K_,_V_>& l1,const cache_entry<_K_,_V_>& l2){
            return  (l1._key==l2._key);
        }
        ALWAYS_INLINE friend inline size_t
        hash_value(const cache_entry<_K_,_V_>& e) {
            return e._key.hash_code();
        }
    };

    enum class cache_find_error_code{
        cant_found=-1,
        err_value_type=-2,
        std_exception=-3,
        unk_exception=-4
    };

    template<typename _K_,typename _V_>
    class
    cache:boost::noncopyable{
    private:
        using cache_type = bi::unordered_set
                <
                        cache_entry<_K_,_V_>,
                        bi::member_hook
                                <
                                        cache_entry<_K_,_V_>,
                                        typename cache_entry<_K_,_V_>::uset_hook_type,
                                        &cache_entry<_K_,_V_>::_cache_link
                                >,
                        bi::power_2_buckets<true>,
                        bi::constant_time_size<true>
                >;
        using cache_bucket_type=typename cache_type::bucket_type;
        using cache_iterator = typename cache_type::iterator;
        using const_cache_iterator = typename cache_type::const_iterator;
        struct
        compare {
            ALWAYS_INLINE bool
            operator () (const cache_entry<_K_,_V_>& l, const cache_entry<_K_,_V_>& r) const {
                return l==r;
            }
            ALWAYS_INLINE bool
            operator () (const cache_key<_K_>& k, const cache_entry<_K_,_V_>& e) const {
                return k==e._key;
            }
            ALWAYS_INLINE bool
            operator () (const cache_entry<_K_,_V_>& e, const cache_key<_K_>& k) const {
                return k==e._key;
            }
        };
    private:
        static constexpr size_t initial_bucket_count = 1<<20;
        static constexpr float load_factor = 0.75f;
        size_t _resize_up_threshold = static_cast<size_t>(load_factor * initial_bucket_count);
        cache_bucket_type* _buckets;
        cache_type _store;
    public:
        cache(cache&& o)noexcept=delete;
        cache()noexcept
        :_buckets(new cache_bucket_type[initial_bucket_count])
        ,_store(typename cache_type::bucket_traits(_buckets, initial_bucket_count)){
        }
        virtual
        ~cache()= default;
        ALWAYS_INLINE bool
        erase(const cache_key<_K_>& k){
            static auto hash_fn=[](const cache_key<_K_>& k){ return k.hash_code();};
            auto it=_store.find(k,hash_fn,compare());
            if(it==_store.end())
                return false;
            _store.erase_and_dispose(it,[](cache_entry<_K_,_V_>* o){delete o;});
            return true;
        }

        ALWAYS_INLINE bool
        erase(_K_ k){
            return erase(cache_key<_K_>(k));
        }

        ALWAYS_INLINE void
        push(cache_entry<_K_,_V_>* e, bool delete_when_swap= true){
            auto p=_store.insert(*e);
            if(!p.second){
                std::swap(e->_shared_ptr_data,p.first->_shared_ptr_data);
                if(delete_when_swap)
                    delete e;
            }
            maybe_rehash();
        }

        ALWAYS_INLINE void
        push(_K_ k,const std::shared_ptr<_V_>& v){
            push(new cache_entry<_K_,_V_>(k,v));
        }

        ALWAYS_INLINE std::variant<cache_find_error_code,std::shared_ptr<_V_>>
        find(const cache_key<_K_>& k)noexcept{
            static auto hash_fn=[](const cache_key<_K_>& k){ return k.hash_code();};
            auto it=_store.find(k,hash_fn,compare());
            std::variant<cache_find_error_code,std::shared_ptr<_V_>> res;
            if(it==_store.end()){
                res.template emplace<cache_find_error_code>(cache_find_error_code::cant_found);
                return res;
            }
            else{
                try {
                    res.template emplace<std::shared_ptr<_V_>>(it->_shared_ptr_data);
                }
                catch(std::bad_variant_access& e){
                    res.template emplace<cache_find_error_code>(cache_find_error_code::err_value_type);
                }
                catch(std::exception& e){
                    res.template emplace<cache_find_error_code>(cache_find_error_code::std_exception);
                }
                catch(...){
                    res.template emplace<cache_find_error_code>(cache_find_error_code::unk_exception);
                }
                return res;
            }
        }

        ALWAYS_INLINE std::variant<cache_find_error_code,std::shared_ptr<_V_>>
        find(_K_ k)noexcept{
            return find(cache_key<_K_>(k));
        }
        void maybe_rehash()
        {
            if (_store.size() >= _resize_up_threshold) {
                auto new_size = _store.bucket_count() * 2;
                auto old_buckets = _buckets;
                try {
                    _buckets = new cache_bucket_type[new_size];
                } catch (const std::bad_alloc& e) {
                    return;
                }
                _store.rehash(typename cache_type::bucket_traits(_buckets, new_size));
                delete[] old_buckets;
                _resize_up_threshold = _store.bucket_count() * load_factor;
            }
        }

        friend void operator &(cache_entry<_K_,_V_>& v){}
    };
}
#endif //PROJECT_CAHCE_HH
