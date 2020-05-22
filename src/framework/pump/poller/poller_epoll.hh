//
// Created by null on 19-12-18.
//

#ifndef PROJECT_EPOLL_HH
#define PROJECT_EPOLL_HH


#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#include <functional>

namespace pump::poller{
    class poller_epoll {
    private:
        int epoll_fd;
    public:
        poller_epoll():epoll_fd(epoll_create1(EPOLL_CLOEXEC)){
        }
        explicit poller_epoll(int fd):epoll_fd(fd){
        }
        ~poller_epoll(){
            ::close(epoll_fd);
        }
        template <typename _F>
        int add_event(int fd,uint32_t events,_F&& _handle){
            struct epoll_event e;
            memset(&e, sizeof (epoll_event),0);
            e.events=events;
            e.data.ptr=new std::function<void(const epoll_event&)>(_handle);
            return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &e);
        }
        int remove_event(int fd){
            struct epoll_event e;
            memset(&e, sizeof (epoll_event),0);
            return epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &e);
        }
        template <typename _F>
        int modify_event(int fd,uint32_t events,_F&& _handle){
            struct epoll_event e;
            bzero(&e, sizeof (epoll_event));
            e.events=events;
            e.data.ptr=new std::function<void(const epoll_event&)>(_handle);
            return epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &e);
        }
        int wait_event(epoll_event* events,int size,int ms){
            return epoll_wait(epoll_fd, events, size, ms);
        }
    };
}



#endif //PROJECT_EPOLL_HH
