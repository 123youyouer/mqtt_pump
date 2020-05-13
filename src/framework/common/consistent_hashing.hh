//
// Created by root on 2020/5/12.
//

#ifndef PUMP_CONSISTENT_HASHING_HH
#define PUMP_CONSISTENT_HASHING_HH
#include <string>
#include <map>
namespace common{
    template <typename _K_,typename _N_>
    class consistent_hash{
        std::map<_K_,_N_,std::less<_K_>> nodes;
    public:
        __always_inline auto
        size()const{
            return nodes.size();
        }
        [[nodiscard]] __always_inline bool
        empty()const{
            return nodes.empty();
        }
        __always_inline auto
        emplace(_N_&& node){
            return nodes.emplace(node.get_key(),node);
        }
        __always_inline auto
        find(const _K_& k){
            return nodes.lower_bound(k);
        }
    };

    template <typename _NODE_TYPE_,
            typename _HASH_FUNC_,
            typename Alloc = std::allocator<std::pair<const typename _HASH_FUNC_::result_type,_NODE_TYPE_>>>
    class consistent_hashing{
    public:
        typedef typename _HASH_FUNC_::result_type size_type;
        typedef std::map<size_type,_NODE_TYPE_,std::less<size_type>,Alloc> map_type;
        typedef typename map_type::value_type value_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef typename map_type::iterator iterator;
        typedef typename map_type::reverse_iterator reverse_iterator;
        typedef Alloc allocator_type;
    private:
        _HASH_FUNC_ hasher_;
        map_type nodes_;
    public:
        std::size_t size() const {
            return nodes_.size();
        }
        bool empty() const {
            return nodes_.empty();
        }
        std::pair<iterator,bool> insert(const _NODE_TYPE_& node) {
            size_type hash = hasher_(node);
            return nodes_.insert(value_type(hash,node));
        }
        void erase(iterator it) {
            nodes_.erase(it);
        }
        iterator find(size_type hash) {
            if(nodes_.empty()) {
                return nodes_.end();
            }

            iterator it = nodes_.lower_bound(hash);

            if (it == nodes_.end()) {
                it = nodes_.begin();
            }

            return it;
        }

        iterator begin() { return nodes_.begin(); }
        iterator end() { return nodes_.end(); }
        reverse_iterator rbegin() { return nodes_.rbegin(); }
        reverse_iterator rend() { return nodes_.rend(); }
    };
}
#endif //PUMP_CONSISTENT_HASHING_HH
