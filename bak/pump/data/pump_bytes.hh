//
// Created by null on 20-2-4.
//

#ifndef PROJECT_PUMP_BYTES_HH
#define PROJECT_PUMP_BYTES_HH

#include <memory>
#include <cstring>

namespace data{
    struct
    pump_bytes{
        uint16_t len;
        uint16_t max;
        const char* buf;
        [[gnu::always_inline]][[gnu::hot]]
        friend inline bool
        operator == (const pump_bytes& l1,const pump_bytes& l2){
            return (l1.len==l2.len) && (std::memcmp(l1.buf,l2.buf,l1.len)==0);
        }
    };


}
#endif //PROJECT_PUMP_BYTES_HH
