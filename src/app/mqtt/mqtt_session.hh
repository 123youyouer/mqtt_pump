//
// Created by null on 20-1-12.
//

#ifndef PROJECT_MQTT_SESSION_HH
#define PROJECT_MQTT_SESSION_HH

#include "net/tcp_session.hh"

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
    }; // namespace control_packet_type
    enum class
    mqtt_parse_state_code{
        ok              =0,
        read_more       =1,
        proto_error     =-1,
        net_error       =-2
    };
    enum class
    mqtt_protocol_version {
        undetermined  = 0,
        v3_1_1        = 4,
        v5            = 5,
    };
    struct
    mqtt_command:public boost::noncopyable{
        char* data;
        uint16_t data_len;
        virtual mqtt_parse_state_code from_ringbuffer(net::linear_ringbuffer_st* buf)=0;
        uint8_t
        command_type(){
            return uint8_t((*((uint8_t*)data))&0b11110000);
        }
        uint8_t
        command_head_attachment(){
            return uint8_t((*((uint8_t*)data))&0b00001111);
        }
        mqtt_command(mqtt_command&& o)noexcept:data(o.data),data_len(o.data_len){
            o.data=nullptr;
            o.data_len=0;
        }
        mqtt_command():data(nullptr),data_len(0){};
    };
    struct
    mqtt_packet_connect : public mqtt_command{
        static constexpr std::size_t command_len =
                1 +  // command type
                2 +  // string length
                4 +  // "MQTT" string
                1 +  // ProtocolVersion
                1 +  // ConnectFlag
                2;   // KeepAlive
        mqtt_parse_state_code from_ringbuffer(net::linear_ringbuffer_st* buf){
            if(buf->size()< command_len)
                return mqtt_parse_state_code::read_more;
            char* data=new char[command_len];
            data_len=command_len;
            memcpy(data,buf,command_len);
            if(command_type()!= static_cast<uint8_t>(mqtt_command_type::connect))
                return mqtt_parse_state_code::proto_error;
            if(command_head_attachment()!=0)
                return mqtt_parse_state_code::proto_error;
            if(((*((uint16_t*)(data+1))))!=0x04)
                return mqtt_parse_state_code::proto_error;
            if(data[3]!='M'||data[4]!='Q'||data[5]!='T'||data[6]!='T')
                return mqtt_parse_state_code::proto_error;
            if(data[7]!= static_cast<uint8_t>(mqtt_protocol_version::v5))
                return mqtt_parse_state_code::proto_error;
            return mqtt_parse_state_code::ok;

        }
        uint8_t connect_flag(){
            return *((uint8_t*)(data+8));
        }
        uint16_t keep_alive(){
            return *((uint16_t*)(data+9));
        }
        mqtt_packet_connect():mqtt_command(){};

    };
    struct
    mqtt_packet_connack{
        uint8_t head;
        size_t from_ringbuffer(net::linear_ringbuffer_st* buf){
            return 0;
        }
        int check(){
            return head;
        }
        size_t len;
        const char* data;
    };


    reactor::flow_builder<mqtt_parse_state_code>
    send_connect(net::tcp_session& session,mqtt_packet_connack&& pkt){
        //return session.send_data(nullptr,10);
        return reactor::make_flow([]()mutable{
            return mqtt_parse_state_code::proto_error;
        });
    }

    reactor::flow_builder<mqtt_parse_state_code>
    wait_connect(net::tcp_session& session){
        return session.wait_packet(mqtt_packet_connect::command_len)
                .then([empty_pkt = mqtt_packet_connect(), &session](net::linear_ringbuffer_st *buf)mutable {
                    switch (empty_pkt.from_ringbuffer(buf)) {
                        case mqtt_parse_state_code::ok:
                            buf->consume(empty_pkt.data_len);
                            return send_connect(session, mqtt_packet_connack());
                        case mqtt_parse_state_code::read_more:
                        case mqtt_parse_state_code::proto_error:
                        default:
                            return reactor::make_flow([]()mutable {
                                return mqtt_parse_state_code::proto_error;
                            });
                    }
                })
                .when_exception([](std::exception_ptr p) {
                    try {
                        if (p)
                            std::rethrow_exception(p);
                    }
                    catch (const net::tcp_session_exception &e) {
                        switch (e.code) {
                            case net::tcp_session_exception::code_s::read_error:
                                return std::optional<mqtt_parse_state_code>(mqtt_parse_state_code::net_error);
                            default:
                                return std::optional<mqtt_parse_state_code>(mqtt_parse_state_code::proto_error);
                        }
                    }
                    catch (const std::exception &e) {
                        std::cout << e.what() << std::endl;
                        return std::optional<mqtt_parse_state_code>(mqtt_parse_state_code::proto_error);
                    }
                    catch (...) {
                        std::cout << "PPPPPPPPPPPPPPPPPPPPPPPPPP" << std::endl;
                        return std::optional<mqtt_parse_state_code>(mqtt_parse_state_code::proto_error);
                    }
                });
    }

}
#endif //PROJECT_MQTT_SESSION_HH
