//
// Created by null on 20-1-31.
//

#ifndef PROJECT_AIO_FILE_HH
#define PROJECT_AIO_FILE_HH


#include <fcntl.h>
#include <libaio.h>
#include <hw/cpu.hh>
#include <sys/eventfd.h>
#include <poller/poller.hh>
#include <reactor/flow.hh>
#include <moodycamel/concurrentqueue.h>
#include <variant>
#include <utils/noncopyable_function.hh>

namespace aio{
    struct aio_cb_args{
        long res1;
        long res2;
        explicit
        aio_cb_args(long r1,long r2):res1(r1),res2(r2){}
    };
    using _aio_cb_event_type_=std::variant<std::monostate,aio::aio_cb_args>;
}
namespace reactor{
    template <>
    class schedule_able<aio::_aio_cb_event_type_>{

    public:
        using _running_context_type_=reactor::_running_context_type_none_;
        using _schedule_function_arg_type_=aio::_aio_cb_event_type_;
        constexpr static _running_context_type_ _default_running_context_=_running_context_type_none_();
    public:
        _running_context_type_ _rc_;
        schedule_able():_rc_(_running_context_type_none_()){}
        void set_schedule_context(const _running_context_type_& c){
            _rc_=c;
        }
    };
}
namespace aio{

    constexpr const uint32_t max_events=16;

    namespace _private{
        thread_local moodycamel::ConcurrentQueue<iocb*> cached_iocbs;
        thread_local struct io_event aio_events[max_events];
    }


    class
    aio_file_flow : public reactor::flow<_aio_cb_event_type_,aio_cb_args,true>{
        utils::noncopyable_function<void()> aio_submit_func;
    public:
        [[gnu::always_inline]][[gnu::hot]]
        void
        submit() override {
            aio_submit_func();
        }
        template <typename _F>
        explicit
        aio_file_flow(_F&& f):aio_submit_func(f),reactor::flow<_aio_cb_event_type_,aio_cb_args,true>([](_aio_cb_event_type_&& a)mutable {
            return std::forward<aio_cb_args>(std::get<1>(a));
        }){}
    };

    class
    aio_file{
    private:
        int _file_fd;
        int _event_fd;
        io_context_t ctx;
        std::string s;
    public:
        explicit
        aio_file(const char* path):ctx(){
            io_setup(max_events,&ctx);
            _file_fd=open(path,O_RDWR|O_CREAT|O_APPEND,0664);
            _event_fd=eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
            s=path;

        }
        virtual
        ~aio_file(){
            close(_file_fd);
            close(_event_fd);
        }
        void
        on_event(const epoll_event& e){
            int num=io_getevents(ctx,0,max_events,_private::aio_events, nullptr);
            for (int i = 0; i < num; i++) {
                if(!_private::aio_events->data){
                    continue;
                }
                else{
                    static_cast<aio_file_flow*>(_private::aio_events[i].data)->
                            active(aio_cb_args(_private::aio_events[i].res,_private::aio_events[i].res2));
                }
            }
        }
        void
        bind_cpu(hw::cpu_core cpu){
            poller::_all_pollers[static_cast<int>(cpu)]
                    ->add_event(
                            _event_fd,
                            EPOLLIN|EPOLLOUT|EPOLLET,
                            [this](const epoll_event& e){
                                on_event(e);
                            });
        }
        void
        write_no_res(void* buf,int len,int offset=0){
            iocb io;
            io_prep_pwrite(&io,_file_fd,buf, len,offset);
            io.data=nullptr;
            struct iocb* cmds[1] = { &io };
            io_submit(ctx,1,cmds);
        }

        auto
        wait_write_done(void* buf,int len,int offset=0){
            iocb* io=new iocb;
            io_prep_pwrite(io,_file_fd,buf, len,offset);
            io_set_eventfd(io,_event_fd);
            aio_file_flow* f=new aio_file_flow([this,io]()mutable{
                ::io_submit(ctx,1,&io);
                delete io;
            });
            io->data=(void*)(f);
            return reactor::flow_builder(f);
        }
        auto
        wait_read_done(void* buf,int len,int offset){
            iocb* io=new iocb;
            io_prep_pread(io,_file_fd,buf, len,offset);
            io_set_eventfd(io,_event_fd);
            aio_file_flow* f=new aio_file_flow([this,io]()mutable{
                ::io_submit(ctx,1,&io);
                delete io;
            });
            io->data=(void*)(f);
            return reactor::flow_builder(f);
        }
    };
}
#endif //PROJECT_AIO_FILE_HH
