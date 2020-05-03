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

namespace resp{
    static const char reply_string = '+';
    static const char reply_error = '-';
    static const char reply_integer = ':';
    static const char reply_bulk = '$';
    static const char reply_array = '*';
    template<int T>
    class redis_command{
        int
        get_type(){
            return T;
        }
    };
    class resp_decode_exception : public std::logic_error{
    public:
        resp_decode_exception():std::logic_error("decode response error"){}
    };
    enum class reply_type{
        v_string,
        v_err,
        v_integer,
        v_bulk,
        v_bulk_null,
        v_bulk_empty,
        v_array,
        v_size,
    };
    struct reply:boost::noncopyable{
        reply_type typ;
        size_t bgn;
        size_t end;
        size_t children_len;
        reply* children;
    };
    class decoder{
        enum class value_type{
            v_string,
            v_err,
            v_integer,
            v_bulk,
            v_bulk_null,
            v_bulk_empty,
            v_array,
            v_size,
        };
        enum class state
        {
            st_start,
            st_string,
            st_error_string,
            st_integer,
            st_bulk_size,
            st_bulk,
            st_array_size,
            st_array_size_lf,
            st_complete,
            st_incomplete,
            st_decode_error,
        };
        struct value_anchor{
            value_type typ;
            size_t bgn;
            size_t end;
        };
    private:
        __always_inline static bool
        ischar(int c){
            return c >= 0 && c <= 127;
        }

        __always_inline static bool
        isctrl(int c){
            return (c >= 0 && c <= 31) || (c == 127);
        }
    public:
        decoder(){}
        state
        decode_chunk_at_st_start(char const* ptr, size_t& offset,  size_t size){
            switch (ptr[offset++]){
                case reply_string:
                    return state::st_string;
                case reply_error:
                    return state::st_error_string;
                case reply_integer:
                    return state::st_integer;
                case reply_bulk:
                    return state::st_bulk_size;
                case reply_array:
                    return state::st_array_size;
                default:
                    return state::st_decode_error;
            }
        }

        state
        decode_chunk_at_st_string(reply& v, bool is_err, char const* ptr, size_t& offset, size_t size){
            v.typ=is_err?reply_type::v_err:reply_type::v_string;
            v.bgn=offset;
            v.children=nullptr;
            v.children_len=0;
            bool cr_geted= false;
            for(;offset<size;++offset){
                char c=ptr[offset];
                switch (c){
                    case '\r':
                        cr_geted= true;
                        break;
                    case '\n':
                        if(cr_geted)
                            return ++offset,state::st_complete;
                        else
                            return state::st_decode_error;
                    default:
                        if(ischar(c) && !isctrl(c))
                            v.end=offset;
                        else
                            return state::st_decode_error;
                        break;
                }
            }
            return state::st_incomplete;
        }
        state
        decode_chunk_at_st_integer(reply& v, char const* ptr, size_t& offset, size_t size){
            v.typ=reply_type::v_integer;
            v.bgn=offset;
            v.children=nullptr;
            v.children_len=0;
            bool cr_geted= false;
            for(;offset<size;++offset){
                char c=ptr[offset];
                switch (c){
                    case '\r':
                        cr_geted= true;
                        break;
                    case '\n':
                        if(cr_geted)
                            return ++offset,state::st_complete;
                        else
                            return state::st_decode_error;
                    default:
                        if(isdigit(c) || c=='-')
                            v.end=offset;
                        else
                            return state::st_decode_error;
                        break;
                }
            }
            return state::st_incomplete;
        }

        state
        decode_size(reply& v, char const* ptr, size_t& offset, size_t size){
            v.typ=reply_type::v_integer;
            v.bgn=offset;
            bool cr_geted= false;
            for(;offset<size;++offset){
                char c=ptr[offset];
                switch (c){
                    case '\r':
                        cr_geted= true;
                        break;
                    case '\n':
                        if(cr_geted)
                            return ++offset,state::st_complete;
                        else
                            return state::st_decode_error;
                    default:
                        if(isdigit(c) || c=='-')
                            v.end=offset;
                        else
                            return state::st_decode_error;
                        break;
                }
            }
            return state::st_incomplete;

        }
        state
        decode_chunk_at_st_bulk(reply& v,char const* ptr, size_t& offset, size_t size){
            auto s=decode_size(v,ptr,offset,size);
            switch (s){
                case state::st_complete:
                default:
                    return s;
            }
            if(value.typ!=value_type::v_size)
                return state::st_decode_error;

            auto bulk_size=std::strtol(ptr+value.bgn, nullptr,10);

            if(bulk_size<0){
                if(offset+2>=size)
                    return state::st_incomplete;
                value.typ=value_type::v_bulk_null;
                if(ptr[++offset]!='\r')
                    return state::st_decode_error;
                if(ptr[++offset]!='\n')
                    return state::st_decode_error;
                return state::st_complete;
            }
            else if (0 == bulk_size){
                if(offset+2>=size)
                    return state::st_incomplete;
                value.typ=value_type::v_bulk_empty;
                if(ptr[++offset]!='\r')
                    return state::st_decode_error;
                if(ptr[++offset]!='\n')
                    return state::st_decode_error;
                return state::st_complete;
            }
            else{
                if(offset+bulk_size+2>=size)
                    return state::st_incomplete;
                value.typ=value_type::v_bulk;
                value.bgn=offset;
                offset+=bulk_size;
                value.end=offset;
                if(ptr[++offset]!='\r')
                    return state::st_decode_error;
                if(ptr[++offset]!='\n')
                    return state::st_decode_error;
                return state::st_complete;
            }
        }

        state
        decode_chunk_by_state(state s,const char* ptr, size_t& offset, size_t size){
            switch (s){
                case state::st_start:
                    return decode_chunk_at_st_start(ptr,offset,size);
                case state::st_string:
                    return decode_chunk_at_st_string(false,ptr,offset,size);
                case state::st_error_string:
                    return decode_chunk_at_st_string(true,ptr,offset,size);
                case state::st_integer:
                    return decode_chunk_at_st_integer(ptr,offset,size);
                case state::st_bulk_size:
                    return decode_chunk_at_st_bulk_size(ptr,offset,size);
                case state::st_bulk:
                    return decode_chunk_at_st_bulk(ptr,offset,size);
                case state::st_array_size:
                case state::st_array_size_lf:
                case state::st_complete:
                    return decode_chunk_at_st_start(ptr,offset,size);
                default:
                    return state::st_decode_error;
            }
        }
        state
        decode_chunk(char const* ptr, size_t size){
            state s=state::st_start;
            size_t offset=0;
            while(s!=state::st_complete
                  && s!=state::st_incomplete
                  && s!=state::st_decode_error)
                s=decode_chunk_by_state(s,ptr,offset,size);
            return s;
    };
}
#endif //PUMP_RESP_DECODER_HH
