//
// Created by null on 20-1-17.
//

#ifndef PROJECT_THREADING_STATE_HH
#define PROJECT_THREADING_STATE_HH
namespace threading{
    struct thread_state{
        int wait_start_flag;
        int running_step;
        int epoll_wait_time;
    };
    thread_state _all_thread_state_[128];
}
#endif //PROJECT_THREADING_STATE_HH
