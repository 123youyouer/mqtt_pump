//
// Created by root on 2020/3/3.
//

#ifndef PUMP_MQTT_PACKET_HH
#define PUMP_MQTT_PACKET_HH
namespace mqtt{
    enum class
    mqtt_command_type : std::uint8_t {
        reserved    = 0b00000000,
        connect     = 0b00010000, // 1
        connack     = 0b00100000, // 2
        publish     = 0b00110000, // 3
        puback      = 0b01000000, // 4
        pubrec      = 0b01010000, // 5
        pubrel      = 0b01100000, // 6
        pubcomp     = 0b01110000, // 7
        subscribe   = 0b10000000, // 8
        suback      = 0b10010000, // 9
        unsubscribe = 0b10100000, // 10
        unsuback    = 0b10110000, // 11
        pingreq     = 0b11000000, // 12
        pingresp    = 0b11010000, // 13
        disconnect  = 0b11100000, // 14
        auth        = 0b11110000, // 15
    };
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
    struct mqtt_pkt_connack:boost::noncopyable{
        struct _inner_data{
            u_int8_t        cmd{};
            u_int8_t        remaining{};
            u_int8_t        acknowledge_flags{};
            u_int8_t        return_code{};
            _inner_data()
            :cmd(static_cast<u_int8_t>(mqtt_command_type::connack))
            ,remaining(0x02){
            }
        };
        std::shared_ptr<_inner_data> data;
        ALWAYS_INLINE const u_int8_t*
        to_buf(){
            return new u_int8_t[4]{data->cmd,data->remaining,data->acknowledge_flags,data->return_code};
        }
        ALWAYS_INLINE static size_t
        len(){
            return 4;
        }
        ALWAYS_INLINE static const u_int8_t*
        make_buf(u_int8_t acknowledge_flags,u_int8_t return_code){
            return new u_int8_t[4]{static_cast<u_int8_t>(mqtt_command_type::connack),0x02,acknowledge_flags,return_code};
        }
        mqtt_pkt_connack():data(std::make_shared<_inner_data>()){}
    };
    struct mqtt_pkt_connect:boost::noncopyable{
        struct _inner_data{
            u_int8_t        cmd{};
            u_int32_t       remaining{};
            u_int8_t        protocol_name[6]{};
            u_int8_t        protocol_level{};
            u_int8_t        connect_flags{};
            u_int16_t       keep_alive{};
            std::string     client_id;
            std::string     will_topic;
            u_int8_t*       will_message{};
            std::string     username;
            u_int8_t*       passward{};

            ALWAYS_INLINE bool
            check_protocol_name(){
                return protocol_name[0]== 0 &&
                       protocol_name[1]== 4 &&
                       protocol_name[2]=='M' &&
                       protocol_name[3]=='Q' &&
                       protocol_name[4]=='T' &&
                       protocol_name[5]=='T' ;
            }

            ALWAYS_INLINE bool
            check_clean_session(){
                return (protocol_level&0x0b00000010)!=0;
            }

            ALWAYS_INLINE bool
            check_protocol_level(){
                return (protocol_level&0x7f)==4;
            }
            ALWAYS_INLINE bool
            check_connect_flags(){
                return (connect_flags&0x01)!=0x00;
            }

            bool
            read_fixed_head(common::ringbuffer* buf){
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

            bool
            read_payload(common::ringbuffer* buf){
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

            bool
            from_buf(common::ringbuffer* buf){
                if(!read_fixed_head(buf))
                    return false;
                return read_payload(buf);
            }
        };

        std::shared_ptr<_inner_data> data;

        mqtt_pkt_connect()= default;
        explicit
        mqtt_pkt_connect(u_int8_t cmd,u_int32_t remaining,common::ringbuffer* others):data(new _inner_data()){
            this->data->cmd=cmd;
            this->data->remaining=remaining;
            if(this->data->from_buf(others))
                throw std::logic_error("cant build connect packet");
        }
        mqtt_pkt_connect(mqtt_pkt_connect&& o)noexcept:data(std::move(o.data)){
        }
    };
    struct mqtt_pkt_publish:boost::noncopyable{
        struct _inner_data{
            u_int8_t        cmd{};
            u_int32_t       remaining{};
            std::string     topic_name;
            u_int16_t       packet_id{};
            u_int8_t*       payload{};

            u_int8_t
            qos(){ return (cmd&0b00000110)>>1; }
            u_int8_t
            dup(){ return (cmd&0b00001000)>>3; }
            u_int8_t
            retain(){ return (cmd&0b00000001); }
            ~_inner_data(){
                if(payload)
                    delete []payload;
            }
            bool from_buf(common::ringbuffer* buf){
                buf->pop_string(topic_name);
                buf->pop_uint_16(packet_id);
                int l=remaining-2-topic_name.size()-2;
                payload=new u_int8_t[l];
                buf->pop_bytes(payload,l);
                return true;
            }
        };
        std::shared_ptr<_inner_data> data;

        explicit
        mqtt_pkt_publish(u_int8_t cmd,u_int32_t remaining,common::ringbuffer* others):data(new _inner_data()){
            data->cmd=cmd;
            data->remaining=remaining;
            data->from_buf(others);
        }
        mqtt_pkt_publish(mqtt_pkt_publish&& o)noexcept:data(std::move(o.data)){
        }
    };

    struct mqtt_pkt_suback{
        struct _inner_data{
            u_int8_t cmd{};
            u_int8_t remaining{};
            u_int16_t packet_id{};
        };
        std::shared_ptr<_inner_data> data;
        ALWAYS_INLINE static const u_int8_t*
        make_buf(u_int16_t packet_id){
            u_int8_t* r=new u_int8_t[4];
            r[0]=static_cast<u_int8_t>(mqtt_command_type::suback);
            r[1]=0x02;
            std::memcpy(r+2,&packet_id,2);
            return r;
        }
        ALWAYS_INLINE static size_t
        len(){
            return 4;
        }
    };
    struct topic_subscribe_info{
        std::string topic;
        u_int8_t    qos{};
    };
    struct mqtt_pkt_subscribe:boost::noncopyable{
        struct _inner_data{
            u_int8_t cmd{};
            u_int32_t remaining{};
            u_int16_t packet_id{};
            std::vector<const topic_subscribe_info*> topic_subscribe_infos;
            ~_inner_data(){
                for(auto* e:topic_subscribe_infos)
                    delete e;
            }
            bool from_buf(common::ringbuffer* buf){
                buf->pop_uint_16(packet_id);
                auto bgn= buf->read_head();
                while (buf->read_head()>(bgn+remaining-2)){
                    auto* new_info=new topic_subscribe_info();
                    buf->pop_string(new_info->topic);
                    buf->pop_uint_8(new_info->qos);
                    topic_subscribe_infos.push_back(new_info);
                }
                return true;
            }
        };

        std::shared_ptr<_inner_data> data;

        explicit
        mqtt_pkt_subscribe(u_int8_t cmd,u_int32_t remaining,common::ringbuffer* others):data(new _inner_data()){
            data->cmd=cmd;
            data->remaining=remaining;
            data->from_buf(others);
        }
        mqtt_pkt_subscribe(mqtt_pkt_subscribe&& o)noexcept:data(std::move(o.data)){
        }
    };

    struct mqtt_pkt_unsuback{
        struct _inner_data{
            u_int8_t cmd{};
            u_int8_t remaining{};
            u_int16_t packet_id{};
        };
        std::shared_ptr<_inner_data> data;
        ALWAYS_INLINE static const u_int8_t*
        make_buf(u_int16_t packet_id){
            u_int8_t* r=new u_int8_t[4];
            r[0]=static_cast<u_int8_t>(mqtt_command_type::unsuback);
            r[1]=0x02;
            std::memcpy(r+2,&packet_id,2);
            return r;
        }
        ALWAYS_INLINE static size_t
        len(){
            return 4;
        }
    };
    struct topic_unsubscribe_info{
        std::string topic;
        u_int8_t    qos{};
    };
    struct mqtt_pkt_unsubscribe:boost::noncopyable{
        struct _inner_data{
            u_int8_t cmd{};
            u_int32_t remaining{};
            u_int16_t packet_id{};
            std::vector<const topic_unsubscribe_info*> topic_unsubscribe_infos;
            ~_inner_data(){
                for(auto* e:topic_unsubscribe_infos)
                    delete e;
            }
            bool from_buf(common::ringbuffer* buf){
                buf->pop_uint_16(packet_id);
                auto bgn= buf->read_head();
                while (buf->read_head()>(bgn+remaining-2)){
                    auto* new_info=new topic_unsubscribe_info();
                    buf->pop_string(new_info->topic);
                    buf->pop_uint_8(new_info->qos);
                    topic_unsubscribe_infos.push_back(new_info);
                }
                return true;
            }
        };

        std::shared_ptr<_inner_data> data;

        explicit
        mqtt_pkt_unsubscribe(u_int8_t cmd,u_int32_t remaining,common::ringbuffer* others):data(new _inner_data()){
            data->cmd=cmd;
            data->remaining=remaining;
            data->from_buf(others);
        }
        mqtt_pkt_unsubscribe(mqtt_pkt_unsubscribe&& o)noexcept:data(std::move(o.data)){
        }
    };

    struct mqtt_pkt_pong{
        ALWAYS_INLINE static const u_int8_t*
        make_buf(){
            return new u_int8_t[2]{static_cast<u_int8_t>(mqtt_command_type::pingresp),0};
        }
        ALWAYS_INLINE static size_t
        len(){
            return 2;
        }
    };
}
#endif //PUMP_MQTT_PACKET_HH
