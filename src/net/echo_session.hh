//
// Created by null on 19-12-21.
//

#ifndef PROJECT_ECHO_SESSION_HH
#define PROJECT_ECHO_SESSION_HH

#include <poller/poller.hh>
#include <net/listener.hh>

namespace net{
    class echo_session{
    public:
        explicit echo_session(const session_data& session){

        }
        auto wait_recved(){

        }
        auto wait_writeable(){

        }
    };
}
#endif //PROJECT_ECHO_SESSION_HH
