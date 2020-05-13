//
// Created by root on 2020/5/3.
//

#ifndef PUMP_RESP_TEST_HH
#define PUMP_RESP_TEST_HH

#include <gtest/gtest.h>
#include <cstring>
#include <redis_agent/resped/resp_reply_decoder.hh>

namespace redis_agent::test{
    resp::decode::decode_state
    reply_decode(const char* str_reply){
        resp::decode::decode_result r;
        size_t offset=0;
        auto s=resp::decode::decode(r,str_reply,offset,std::strlen(str_reply));
        return s;
    }
    TEST(resp_test, test_reply_decode) {
        EXPECT_EQ(reply_decode("*2\r\n$3\r\nKKK\r\n$2\r\nRR\r\n"), resp::decode::decode_state::st_complete);
        EXPECT_EQ(reply_decode("*2\r\n$0\r\n\r\n$2\r\nRR\r\n"), resp::decode::decode_state::st_complete);
        EXPECT_EQ(reply_decode("*2\r\n$-1\r\n\r\n$2\r\nRR\r\n"), resp::decode::decode_state::st_complete);
        EXPECT_EQ(reply_decode("*2\r\n+OK\r\n-ERR\r\n"), resp::decode::decode_state::st_complete);
        EXPECT_EQ(reply_decode("*2\r\n+OK\r\n-ER"), resp::decode::decode_state::st_incomplete);
        EXPECT_EQ(reply_decode("*2\r\n$\r\nKK\r\n$2\r\nRR\r\n"), resp::decode::decode_state::st_decode_error);
        EXPECT_EQ(reply_decode("*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n"),resp::decode::decode_state::st_complete);
    }
}

#endif //PUMP_RESP_TEST_HH
