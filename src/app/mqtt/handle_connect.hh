//
// Created by root on 2020/3/3.
//

#ifndef PUMP_HANDLE_CONNECT_HH
#define PUMP_HANDLE_CONNECT_HH

#include <boost/noncopyable.hpp>
namespace mqtt{
    enum class qos : std::uint8_t{
        at_most_once    = 0b00000000,
        at_least_once   = 0b00000001,
        exactly_once    = 0b00000010,
    };
    struct connect_flags {
        static u_int8_t const clean_session  = 0b00000010;
        static u_int8_t const will_flag      = 0b00000100;
        static u_int8_t const will_retain    = 0b00100000;
        static u_int8_t const password_flag  = 0b01000000;
        static u_int8_t const user_name_flag = static_cast<char>(0b10000000u);

        ALWAYS_INLINE static bool
        has_clean_session(char v) {
            return (v & clean_session) != 0;
        }
        ALWAYS_INLINE static bool
        has_will_flag(char v) {
            return (v & will_flag) != 0;
        }
        ALWAYS_INLINE static bool
        has_will_retain(char v) {
            return   ((v & will_retain) != 0);
        }
        ALWAYS_INLINE static bool
        has_password_flag(char v) {
            return (v & password_flag) != 0;
        }
        ALWAYS_INLINE static bool
        has_user_name_flag(char v) {
            return (v & user_name_flag) != 0;
        }
        ALWAYS_INLINE static void
        set_will_qos(char& v, qos qos_value) {
            v |= static_cast<char>(static_cast<std::uint8_t>(qos_value) << 3);
        }
        ALWAYS_INLINE static qos
        will_qos(char v) {
            return static_cast<qos>((v & 0b00011000) >> 3);
        }
    };

    struct mqtt_pkt_connect{
        struct _inner_data{
            u_int32_t       remaining;
            u_int8_t        protocol_name[6];
            u_int8_t        protocol_level;
            u_int8_t        connect_flags;
            u_int16_t       keep_alive;
            std::string     client_id;
            std::string     will_topic;
            u_int8_t*       will_message;
            std::string     username;
            u_int8_t*       passward;

            bool check_protocol_name(){
                return protocol_name[0]== 0 &&
                       protocol_name[1]== 4 &&
                       protocol_name[2]=='M' &&
                       protocol_name[3]=='Q' &&
                       protocol_name[4]=='T' &&
                       protocol_name[5]=='T' ;
            }

            bool check_protocol_level(){
                return (protocol_level&0x7f)==4;
            }
            bool check_connect_flags(){
                return (connect_flags&0x01)!=0x00;
            }

            bool read_fixed_head(common::ringbuffer* buf){
                buf->pop_bytes(protocol_name,6);
                if(!check_protocol_name())
                    return false;
                buf->pop_uint_8(protocol_level);
                if(!check_protocol_level())
                    return false;
                buf->pop_uint_8(connect_flags);
                if(!check_connect_flags())
                    return false;
                buf->pop_uint_16(keep_alive);
                return true;
            }

            bool read_payload(common::ringbuffer* buf){
                buf->pop_string(client_id);
                if(connect_flags::has_will_flag(connect_flags)){
                    buf->pop_string(will_topic);
                    u_int16_t len=0;
                    buf->pop_uint_16(len);
                    will_message=new u_int8_t[len+2];
                    std::memcpy(will_message,&len,2);
                    buf->pop_bytes(will_message+2,len);
                }
                if(connect_flags::has_user_name_flag(connect_flags)){
                    buf->pop_string(username);
                }
                if(connect_flags::has_password_flag(connect_flags)){
                    u_int16_t len=0;
                    buf->pop_uint_16(len);
                    passward=new u_int8_t[len+2];
                    std::memcpy(passward,&len,2);
                    buf->pop_bytes(passward+2,len);
                }
                return true;
            }

            bool from_buf(common::ringbuffer* buf){
                if(!read_fixed_head(buf))
                    return false;
                if(!read_payload(buf))
                    return false;
            }
        };

        std::shared_ptr<_inner_data> data;

        explicit
        mqtt_pkt_connect(u_int32_t remaining,common::ringbuffer* buf):data(new _inner_data()){
            this->data->remaining=remaining;
            if(this->data->from_buf(buf))
                throw std::logic_error("cant build connect packet");
        }
        mqtt_pkt_connect(mqtt_pkt_connect&& o)noexcept:data(this->data){
        }
    };
    auto
    handle_connect(mqtt_pkt_connect& pkt){
    }
}
#endif //PUMP_HANDLE_CONNECT_HH
