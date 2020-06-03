//
// Created by root on 2020/5/27.
//

#ifndef PUMP_UNSENT_DATA_HH
#define PUMP_UNSENT_DATA_HH
namespace pump::net{
    struct unsent_data:boost::noncopyable{
        struct data{
            const char*   _buf;
            size_t  _len;
            size_t  _cur;
        };
        std::shared_ptr<data> _data;
        unsent_data()= delete;
        explicit
        unsent_data(const char* buf,size_t len):_data(std::make_shared<data>()){
            _data->_cur=0;
            _data->_buf=buf;
            _data->_len=len;
        }
        explicit
        unsent_data(std::shared_ptr<data>& d){
            _data=d;
        }
        unsent_data(const unsent_data&& o)noexcept{
            _data=o._data;
        }
        unsent_data(unsent_data&& o)noexcept{
            _data=o._data;
        }
        unsent_data&
        operator=(unsent_data&& x) noexcept {
            if (this != &x) {
                this->_data=x._data;
            }
            return *this;
        }
        ALWAYS_INLINE const char*
        head(){
            return _data->_buf+_data->_cur;
        }
        ALWAYS_INLINE const char*
        buf(){
            return _data->_buf;
        }
        ALWAYS_INLINE size_t
        send_len(){
            return _data->_len>_data->_cur?_data->_len-_data->_cur:0;
        }
        ALWAYS_INLINE size_t
        has_sent(){
            return _data->_cur;
        }
        ALWAYS_INLINE bool
        done(){
            return _data->_len<=_data->_cur;
        }
        ALWAYS_INLINE void
        update(size_t t){
            _data->_cur+=t;
        }
        template <typename _F_>
        auto
        send(_F_&& f){
            int l=f(_data->_buf,_data->_len-_data->_cur);
            if(l>0)
                _data->_cur+=l;
            return l;
        }
    };
}
#endif //PUMP_UNSENT_DATA_HH
