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
#include <utils/g_define.hh>

namespace data{
    using namespace boost::hana::literals;
    namespace bi=boost::intrusive;
    class
    cache_key{
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
        PUMP_INLINE size_t
        hash_code()const{
            return _code;
        }
        PUMP_INLINE friend bool
        operator==(const cache_key& l,const cache_key& r){
            return (l._code==r._code)&&(r._view==r._view);
        }
    };

    template <typename ..._VALUE_TYPES_>
    class cache_lsu;

    template <typename ..._VALUE_TYPES_>
    class
    cache_entry{
        friend class cache_lsu<_VALUE_TYPES_...>;
    private:
        using variant_type=std::variant<std::monostate,std::shared_ptr<_VALUE_TYPES_>...>;
        using list_link_type=bi::list_member_hook<bi::link_mode<bi::auto_unlink>>;
        using uset_hook_type=bi::unordered_set_member_hook<>;
        bi::list_member_hook<> _timer_link;
        uset_hook_type _cache_link;
        list_link_type _dirty_link;
        list_link_type _lslru_link;
    public:
        cache_key _key;
        std::shared_ptr<variant_type> _shared_ptr_data;
        template <typename _V_> explicit
        cache_entry(const char* _k,const std::shared_ptr<_V_>& v)noexcept
                :_key(_k),_shared_ptr_data(std::make_shared<variant_type>()){
            _shared_ptr_data->template emplace<std::shared_ptr<_V_>>(v);
        }

        PUMP_INLINE friend bool
        operator == (const cache_entry<_VALUE_TYPES_...>& l1,const cache_entry<_VALUE_TYPES_...>& l2){
            return  (l1._key==l2._key);
        }
        [[gnu::always_inline]][[gnu::hot]]
        friend inline size_t
        hash_value(const cache_entry<_VALUE_TYPES_...>& e) {
            return e._key.hash_code();
        }
    };

    enum class cache_find_error_code{
        cant_found=-1,
        err_value_type=-2,
        std_exception=-3,
        unk_exception=-4
    };

    template<typename ..._VALUE_TYPES_>
    class
    cache_lsu:boost::noncopyable{
    private:
        using cache_type = bi::unordered_set
                <
                        cache_entry<_VALUE_TYPES_...>,
                        bi::member_hook
                                <
                                        cache_entry<_VALUE_TYPES_...>,
                                        typename cache_entry<_VALUE_TYPES_...>::uset_hook_type,
                                        &cache_entry<_VALUE_TYPES_...>::_cache_link
                                >,
                        bi::power_2_buckets<true>,
                        bi::constant_time_size<true>
                >;
        using cache_bucket_type=typename cache_type::bucket_type;
        using cache_iterator = typename cache_type::iterator;
        using const_cache_iterator = typename cache_type::const_iterator;
        using lruls_list_type = bi::list
                <
                        cache_entry<_VALUE_TYPES_...>,
                        bi::member_hook
                                <
                                        cache_entry<_VALUE_TYPES_...>,
                                        typename cache_entry<_VALUE_TYPES_...>::list_link_type,
                                        &cache_entry<_VALUE_TYPES_...>::_lslru_link
                                >,
                        bi::constant_time_size<false>
                >;
        using dirty_list_type = bi::list
                <
                        cache_entry<_VALUE_TYPES_...>,
                        bi::member_hook<cache_entry
                                <
                                        _VALUE_TYPES_...>,
                                typename cache_entry<_VALUE_TYPES_...>::list_link_type,
                                &cache_entry<_VALUE_TYPES_...>::_dirty_link
                        >,
                        bi::constant_time_size<false>
                >;
        struct
        compare {
            [[gnu::always_inline]][[gnu::hot]]
            inline bool
            operator () (const cache_entry<_VALUE_TYPES_...>& l, const cache_entry<_VALUE_TYPES_...>& r) const {
                return l==r;
            }
            [[gnu::always_inline]][[gnu::hot]]
            inline bool
            operator () (const cache_key& k, const cache_entry<_VALUE_TYPES_...>& e) const {
                return k==e._key;
            }
            [[gnu::always_inline]][[gnu::hot]]
            inline bool operator () (const cache_entry<_VALUE_TYPES_...>& e, const cache_key& k) const {
                return k==e._key;
            }
        };
    private:
        static constexpr size_t initial_bucket_count = 1<<20;
        static constexpr float load_factor = 0.75f;
        size_t _resize_up_threshold = static_cast<size_t>(load_factor * initial_bucket_count);
        cache_bucket_type* _buckets;
        cache_type _store;
        lruls_list_type _lslru;
        dirty_list_type _dirty;

    public:
        cache_lsu(cache_lsu&& o)noexcept=delete;
        cache_lsu()noexcept
        :_buckets(new cache_bucket_type[initial_bucket_count])
        ,_store(typename cache_type::bucket_traits(_buckets, initial_bucket_count))
        ,_lslru()
        ,_dirty(){
        }
        virtual
        ~cache_lsu()= default;
        PUMP_INLINE bool
        erase(const cache_key& k){
            static auto hash_fn=[](const cache_key& k){ return k.hash_code();};
            auto it=_store.find(k,hash_fn,compare());
            if(it==_store.end())
                return false;
            _store.erase_and_dispose(it,[](cache_entry<_VALUE_TYPES_...>* o){delete o;});
            return true;
        }

        PUMP_INLINE bool
        erase(const char* k){
            return erase(cache_key(k));
        }

        PUMP_INLINE void
        push(cache_entry<_VALUE_TYPES_...>* e, bool delete_when_swap= true){
            auto p=_store.insert(*e);
            if(!p.second){
                e->_shared_ptr_data->swap(*(p.first->_shared_ptr_data));
                if(delete_when_swap)
                    delete e;
            }
            maybe_rehash();
        }

        template <typename _V_>
        PUMP_INLINE void
        push(const char* k,const std::shared_ptr<_V_>& v){
            push(new cache_entry<_VALUE_TYPES_...>(k,v));
        }

        PUMP_INLINE std::variant<cache_find_error_code,std::shared_ptr<typename cache_entry<_VALUE_TYPES_...>::variant_type>>
        find(const cache_key& k)noexcept{
            static auto hash_fn=[](const cache_key& k){ return k.hash_code();};
            auto it=_store.find(k,hash_fn,compare());
            std::variant<cache_find_error_code,std::shared_ptr<typename cache_entry<_VALUE_TYPES_...>::variant_type>> res;
            if(it==_store.end()){
                res.template emplace<cache_find_error_code>(cache_find_error_code::cant_found);
                return res;
            }
            else{
                try {
                    res.template emplace<std::shared_ptr<typename cache_entry<_VALUE_TYPES_...>::variant_type>>(it->_shared_ptr_data);
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

        PUMP_INLINE std::variant<cache_find_error_code,std::shared_ptr<typename cache_entry<_VALUE_TYPES_...>::variant_type>>
        find(const char* k)noexcept{
            return find(cache_key(k));
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

        friend void operator &(cache_lsu<_VALUE_TYPES_...>& v){}
    };
}
#endif //PROJECT_CAHCE_HH
