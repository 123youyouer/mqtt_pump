
//
// Created by root on 2020/5/1.
//

#ifndef PUMP_RESP_DECODER_HH
#define PUMP_RESP_DECODER_HH

#include <cstddef>
#include <stdexcept>
#include <common/g_define.hh>
#include <vector>
#include <boost/core/noncopyable.hpp>

namespace redis_agent::resp{
    static const char reply_string = '+';
    static const char reply_error = '-';
    static const char reply_integer = ':';
    static const char reply_bulk = '$';
    static const char reply_array = '*';
    enum class decode_type{
        v_string,
        v_err_string,
        v_integer,
        v_bulk,
        v_bulk_null,
        v_bulk_empty,
        v_array,
        v_array_null,
        v_unknown,
    };
    struct decode_result:boost::noncopyable{
        using CHILD_TYPE=decode_result;
        decode_type typ;
        size_t bgn;
        size_t end;
        size_t children_len;
        decode_result** children;
        decode_result():typ(decode_type::v_unknown),bgn(0),end(0),children_len(0),children(nullptr){}
        decode_result(decode_result&& o)noexcept:typ(o.typ),bgn(o.bgn),end(o.end),children_len(o.children_len),children(o.children){
            o.children_len=0;
            o.children= nullptr;
        };
        __always_inline decode_result&
        set_type(decode_type t){
            typ=t;
            return *this;
        }
        __always_inline decode_result&
        set_anchor(size_t b,size_t e){
            bgn=b,end=e;
            return *this;
        }
        __always_inline decode_result&
        set_children_len(size_t l){
            if(l>0){
                children_len=l;
                children=new decode_result*[l];
            }
            else{
                for(int i=0;i<children_len;++i)
                    if(children[i]){
                        delete(children[i]);
                        children[i]= nullptr;
                    }

                delete[](children);
                children= nullptr;
            }
            return *this;
        }

        decode_result&
        push_child(int slot,decode_result* c){
            if(slot>=children_len)
                return *this;
            if(children[slot]!= nullptr)
                delete children[slot];
            children[slot]=c;
        }

        static decode_result*
        make_child(){
            return new decode_result();
        }

        void
        set(decode_type t,size_t b,size_t e,size_t l,decode_result** c){
            typ=t,bgn=b,end=e,children_len=l,children=c;
        }
    };
    enum class decode_state
    {
        st_complete,
        st_incomplete,
        st_decode_error,
    };

    template <typename _RES_TYPE_>
    decode_state
    decode(_RES_TYPE_& v,char const* ptr,size_t& offset, size_t size)noexcept;

    template<typename _RES_TYPE_>
    class _imp_decode_ : boost::noncopyable {
    private:
        friend decode_state decode<_RES_TYPE_>(_RES_TYPE_& v,char const* ptr,size_t& offset, size_t size)noexcept;
        __always_inline static bool
        ischar(int c){
            return c >= 0 && c <= 127;
        }

        __always_inline static bool
        isctrl(int c){
            return (c >= 0 && c <= 31) || (c == 127);
        }
        static decode_type
        decide_decode_type(char const* ptr, size_t& offset,  size_t size){
            switch (ptr[offset++]){
                case reply_string:
                    return decode_type::v_string;
                case reply_error:
                    return decode_type::v_err_string;
                case reply_integer:
                    return decode_type::v_integer;
                case reply_bulk:
                    return decode_type::v_bulk;
                case reply_array:
                    return decode_type::v_array;
                default:
                    return decode_type::v_unknown;
            }
        }

        static decode_state
        decode_chunk_string(_RES_TYPE_& res, bool is_err, char const* ptr, size_t& offset, size_t size){
            auto typ=is_err?decode_type::v_err_string:decode_type::v_string;
            auto bgn=offset,end=offset;
            auto children=nullptr;
            auto children_len=0;
            bool cr_geted= false;
            for(;offset<size;++offset){
                char c=ptr[offset];
                switch (c){
                    case '\r':
                        cr_geted= true;
                        break;
                    case '\n':
                        if(cr_geted)
                            return res.set(typ,bgn,end,children_len,children),++offset,decode_state::st_complete;
                        else
                            return decode_state::st_decode_error;
                    default:
                        if(ischar(c) && !isctrl(c))
                            end=offset;
                        else
                            return decode_state::st_decode_error;
                        break;
                }
            }
            return decode_state::st_incomplete;
        }
        static decode_state
        decode_chunk_integer(_RES_TYPE_& res, char const* ptr, size_t& offset, size_t size){
            auto typ=decode_type::v_integer;
            auto bgn=offset,end=offset;
            auto children=nullptr;
            auto children_len=0;
            bool cr_geted= false;
            for(;offset<size;++offset){
                char c=ptr[offset];
                switch (c){
                    case '\r':
                        cr_geted= true;
                        break;
                    case '\n':
                        if(cr_geted)
                            return res.set(typ,bgn,end,children_len,children),++offset,decode_state::st_complete;
                        else
                            return decode_state::st_decode_error;
                    default:
                        if(isdigit(c) || c=='-')
                            end=offset;
                        else
                            return decode_state::st_decode_error;
                        break;
                }
            }
            return decode_state::st_incomplete;
        }

        static decode_state
        decode_size(long& res, char const* ptr, size_t& offset, size_t size){
            auto bgn=offset,end=offset;
            bool cr_geted= false;
            for(;offset<size;++offset){
                char c=ptr[offset];
                switch (c){
                    case '\r':
                        cr_geted= true;
                        break;
                    case '\n':
                        if(cr_geted)
                            if(bgn<=end)
                                return res=std::strtol(ptr+bgn, nullptr,10),++offset,decode_state::st_complete;
                            else
                                return decode_state::st_decode_error;
                        else
                            return decode_state::st_decode_error;
                    default:
                        if(isdigit(c) || c=='-')
                            end=offset;
                        else
                            return decode_state::st_decode_error;
                        break;
                }
            }
            return decode_state::st_incomplete;

        }
        static decode_state
        decode_chunk_bulk(_RES_TYPE_& res,char const* ptr, size_t& offset, size_t size){
            long bulk_size=0;
            switch (decode_size(bulk_size,ptr,offset,size)){
                case decode_state::st_complete:{
                    auto children=nullptr;
                    auto children_len=0;
                    if(bulk_size<0){
                        if(offset+2>=size)
                            return decode_state::st_incomplete;
                        auto typ=decode_type::v_bulk_null;
                        if(ptr[offset]!='\r')
                            return decode_state::st_decode_error;
                        if(ptr[++offset]!='\n')
                            return decode_state::st_decode_error;
                        return res.set(typ,0,0,children_len,children),++offset,decode_state::st_complete;
                    }
                    else if (0 == bulk_size){
                        if(offset+2>=size)
                            return decode_state::st_incomplete;
                        auto typ=decode_type::v_bulk_empty;
                        if(ptr[offset]!='\r')
                            return decode_state::st_decode_error;
                        if(ptr[++offset]!='\n')
                            return decode_state::st_decode_error;
                        return res.set(typ,0,0,children_len,children),++offset,decode_state::st_complete;
                    }
                    else{
                        if(offset+bulk_size-1+2>=size)
                            return decode_state::st_incomplete;
                        auto typ=decode_type::v_bulk;
                        auto bgn=offset,end=offset;
                        offset+=(bulk_size-1);
                        end=offset;
                        if(ptr[++offset]!='\r')
                            return decode_state::st_decode_error;
                        if(ptr[++offset]!='\n')
                            return decode_state::st_decode_error;
                        return res.set(typ,0,0,children_len,children),++offset,decode_state::st_complete;
                    }
                }
                case decode_state::st_incomplete:
                    return decode_state::st_incomplete;
                case decode_state::st_decode_error:
                    return decode_state::st_decode_error;
            }
        }

        static decode_state
        decode_array(_RES_TYPE_& res,char const* ptr, size_t& offset, size_t size){
            long array_size=0;
            switch (decode_size(array_size,ptr,offset,size)){
                case decode_state::st_complete:{
                    if(array_size<0){
                        if(offset+2>=size)
                            return decode_state::st_incomplete;
                        auto typ=decode_type::v_array_null;
                        auto children= nullptr;
                        auto children_len=0;
                        if(ptr[offset]!='\r')
                            return decode_state::st_decode_error;
                        if(ptr[++offset]!='\n')
                            return decode_state::st_decode_error;
                        return res.set(typ,0,0,children_len,children),decode_state::st_complete;
                    }
                    else if (0 == array_size){
                        if(offset+2>=size)
                            return decode_state::st_incomplete;
                        auto typ=decode_type::v_array;
                        auto children= nullptr;
                        auto children_len=0;
                        if(ptr[offset]!='\r')
                            return decode_state::st_decode_error;
                        if(ptr[++offset]!='\n')
                            return decode_state::st_decode_error;
                        return res.set(typ,0,0,children_len,children),decode_state::st_complete;
                    }
                    else{
                        auto typ=decode_type::v_array;
                        auto children_len=array_size;
                        auto children=new typename _RES_TYPE_::CHILD_TYPE*[children_len];
                        for(int i=0;i<array_size;++i){
                            auto* r=_RES_TYPE_::make_child();
                            switch (decode(*r,ptr,offset,size)){
                                case decode_state::st_complete:
                                    children_len++;
                                    children[i]=r;
                                    continue;
                                case decode_state::st_incomplete:
                                    delete r;
                                    return decode_state::st_incomplete;
                                case decode_state::st_decode_error:
                                    delete r;
                                    return decode_state::st_decode_error;
                            }
                        }
                        res.set(typ,0,0,children_len,children);
                        return decode_state::st_complete;
                    }
                }
                case decode_state::st_incomplete:
                    return decode_state::st_incomplete;
                case decode_state::st_decode_error:
                    return decode_state::st_decode_error;
            }
        }
    public:
        _imp_decode_(_imp_decode_&&)=delete;
        _imp_decode_()=delete;
    };
    template <typename _RES_TYPE_>
    decode_state
    decode(_RES_TYPE_& v,char const* ptr,size_t& offset, size_t size)noexcept{
        try{
            switch (_imp_decode_<_RES_TYPE_>::decide_decode_type(ptr,offset,size)){
                case decode_type::v_string:
                    return _imp_decode_<_RES_TYPE_>::decode_chunk_string(v,false,ptr,offset,size);
                case decode_type::v_err_string:
                    return _imp_decode_<_RES_TYPE_>::decode_chunk_string(v,true,ptr,offset,size);
                case decode_type::v_integer:
                    return _imp_decode_<_RES_TYPE_>::decode_chunk_integer(v,ptr,offset,size);
                case decode_type::v_bulk:
                    return _imp_decode_<_RES_TYPE_>::decode_chunk_bulk(v,ptr,offset,size);
                case decode_type::v_array:
                    return _imp_decode_<decode_result>::decode_array(v,ptr,offset,size);
                case decode_type::v_bulk_null:
                case decode_type::v_bulk_empty:
                case decode_type::v_array_null:
                case decode_type::v_unknown:
                default:
                    return decode_state::st_decode_error;
            }
        }
        catch (std::exception& e){
            return decode_state::st_decode_error;
        }
        catch (...){
            return decode_state::st_decode_error;
        }
    }
}
#endif //PUMP_RESP_DECODER_HH