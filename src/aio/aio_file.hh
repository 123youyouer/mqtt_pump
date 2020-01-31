//
// Created by null on 20-1-31.
//

#ifndef PROJECT_AIO_FILE_HH
#define PROJECT_AIO_FILE_HH

#include <hw/cpu.hh>

namespace aio{
    struct
    aio_file{
        int _file_fd;
        int _event_fd;
    };

    void
    start_at(hw::cpu_core cpu){

    }
}
#endif //PROJECT_AIO_FILE_HH
