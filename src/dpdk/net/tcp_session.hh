//
// Created by anyone on 20-2-22.
//

#ifndef PROJECT_TCP_SESSION_HH
#define PROJECT_TCP_SESSION_HH

#include <memory>
#include <list>
#include <functional>
#include <boost/noncopyable.hpp>
#include <f-stack/ff_api.h>
#include <common/g_define.hh>
#include <common/packet_buffer.hh>
#include <common/ncpy_func.hh>
#include <reactor/flow.hh>
#include <reactor/schedule.hh>
#include <timer/time_set.hh>
#include <net/unsent_data.hh>
namespace pump::dpdk::net{
    struct tcp_session:boost::noncopyable{
        using recv_sch_args_t=FLOW_ARG(common::ringbuffer*);
        using send_sch_args_t=FLOW_ARG(pump::net::unsent_data);
        struct recv_handler{
            size_t need_len;
            common::ncpy_func<bool()> available;
            common::ncpy_func<void(FLOW_ARG(common::ringbuffer*)&&)> handle;
        };
        struct data{
            using recv_sch_func_t=recv_handler;
            using send_sch_func_t=common::ncpy_func<void(send_sch_args_t&&)>;
            kevent kevset;
            int state;
            int kd;
            int fd;
            common::ncpy_func<void(kevent& k)> cb;
            std::list<recv_handler> waiting_recv_tasks;
            std::list<std::pair<pump::net::unsent_data,send_sch_func_t>> waiting_send_tasks;
            common::ringbuffer _recv_buf;

            data():state(0),kd(0),fd(0),kevset(),cb(){}

            void
            schedule(recv_handler&& f){
                waiting_recv_tasks.emplace_back(std::forward<recv_sch_func_t>(f));
            }

            void
            schedule(pump::net::unsent_data&& a,send_sch_func_t&& f){
                waiting_send_tasks.emplace_back(std::make_pair(std::forward<pump::net::unsent_data>(a),std::forward<send_sch_func_t>(f)));
                enable_write_event(true);
            }
            void
            close(){
                if(state==0)
                    return;
                state=0;
                EV_SET(&kevset, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
                ff_kevent(kd, &kevset, 1, nullptr, 0, nullptr);
                EV_SET(&kevset, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
                ff_kevent(kd, &kevset, 1, nullptr, 0, nullptr);
                ff_close(fd);
            }
            void
            start(){
                if(state!=0)
                    return;
                state=1;
                EV_SET(&kevset, fd, EVFILT_READ,EV_ADD, 0, 0, &cb);
                ff_kevent(kd, &kevset, 1, nullptr, 0, nullptr);
                EV_SET(&kevset, fd, EVFILT_WRITE,EV_ADD, 0, 0, &cb);
                ff_kevent(kd, &kevset, 1, nullptr, 0, nullptr);
                enable_write_event(false);
            }
            void
            on_send_event(){
                while (!waiting_send_tasks.empty()){
                    auto&& [a,f]=waiting_send_tasks.front();
                    ssize_t n=ff_write(fd,a.head(),a.send_len());
                    if(n>0){
                        a.update(n);
                        if(a.done()){
                            f(send_sch_args_t(std::forward<pump::net::unsent_data>(a)));
                            waiting_send_tasks.pop_front();
                        }
                        else{
                            break;
                        }
                    }
                    else{
                        if(EAGAIN!=errno){
                            f(send_sch_args_t(std::make_exception_ptr(std::logic_error("send pkt error"))));
                            waiting_send_tasks.pop_front();
                        }
                        else{
                            close();
                            break;
                        }
                    }
                }
                enable_write_event(!waiting_send_tasks.empty());
            }
            void
            on_recv_event(){
                auto len= ff_read(fd,_recv_buf.read_head(), _recv_buf.free_size());
                if(len==-1){
                    if(errno==EAGAIN)
                        return;
                    else
                        close();
                }
                else if(len==0){
                    close();
                }
                else{
                    _recv_buf.commit(len);
                    bool still= true;
                    while (still && !waiting_recv_tasks.empty() && !_recv_buf.empty()){
                        auto&& f=waiting_recv_tasks.front();
                        if(f.available()){
                            if(f.need_len>_recv_buf.size()){
                                still= false;
                            }
                            else{
                                f.handle(recv_sch_args_t(&_recv_buf));
                                waiting_recv_tasks.pop_front();
                            }
                        }
                        else{
                            waiting_recv_tasks.pop_front();
                        }
                    }
                }
            }
            void
            enable_write_event(bool enable){
                if(state==0)
                    return;
                EV_SET(&kevset, fd, EVFILT_WRITE,enable?EV_ENABLE:EV_DISABLE, 0, 0, &cb);
                ff_kevent(kd, &kevset, 1, nullptr, 0, nullptr);
            }
        };
        std::shared_ptr<data> _data;
        tcp_session()= delete;
        tcp_session(tcp_session&& o)noexcept{
            _data=o._data;
        };
        tcp_session(const tcp_session&& o)noexcept{
            _data=o._data;
        };
        explicit
        tcp_session(std::shared_ptr<data> d):_data(d){};
        explicit
        tcp_session(int kqfd,int skfd){
            _data=std::make_shared<data>();
            _data->kd=kqfd;
            _data->fd=skfd;
            _data->cb=[_data=_data](kevent& event)
            {
                if(event.flags&EV_EOF){
                    _data->close();
                }
                else if(EVFILT_READ==event.filter){
                    _data->on_recv_event();
                }
                else if(EVFILT_WRITE==event.filter){
                    _data->on_send_event();
                }
            };
            _data->start();
        }

        tcp_session& operator=(tcp_session&& x) noexcept {
            if (this != &x) {
                this->_data=x._data;
            }
            return *this;
        }
        ALWAYS_INLINE auto
        close(){
            return _data->close();
        }
        ALWAYS_INLINE auto
        wait_packet(int ms=0,size_t len=0){
            if(_data->_recv_buf.empty() || _data->_recv_buf.size()<len){
                using f_type=common::ncpy_func<void(FLOW_ARG(std::variant<int,common::ringbuffer*>))>;
                return reactor::flow_builder<std::variant<int,common::ringbuffer*>>::at_schedule
                        (
                                [data=_data,ms,len](std::shared_ptr<reactor::flow_implent<std::variant<int,common::ringbuffer*>>> sp_flow)->void{
                                    data->schedule(tcp_session::recv_handler{
                                            len,
                                            [sp_flow](){ return !sp_flow->called();},
                                            [sp_flow](FLOW_ARG(common::ringbuffer*)&& v){
                                                switch (v.index()){
                                                    case 0:
                                                        sp_flow->trigge(FLOW_ARG(std::variant<int,common::ringbuffer*>)(std::get<0>(v)));
                                                        return;
                                                    case 1:
                                                        sp_flow->trigge(FLOW_ARG(std::variant<int,common::ringbuffer*>)(std::get<1>(v)));
                                                        return;
                                                    default:
                                                        sp_flow->trigge(FLOW_ARG(std::variant<int,common::ringbuffer*>)(
                                                                std::variant<int,common::ringbuffer*>(std::get<2>(v))));
                                                        return;
                                                }
                                            }
                                    });
                                    if(ms<=0)
                                        return;
                                    timer::_sp_timer_set->add_timer(ms,[ms,sp_flow](){
                                        sp_flow->trigge(FLOW_ARG(std::variant<int,common::ringbuffer*>)(
                                                std::variant<int,common::ringbuffer*>(ms)));
                                    });
                                },
                                reactor::_sp_immediate_runner_
                        );
            }
            else{
                return reactor::make_imme_flow(std::variant<int,common::ringbuffer*>(&_data->_recv_buf));
            }
        }
        ALWAYS_INLINE reactor::flow_builder<pump::net::unsent_data>
        send_packet(pump::net::unsent_data&& pxy, std::shared_ptr<reactor::flow_runner>& runner=reactor::_sp_immediate_runner_){
            return reactor::flow_builder<pump::net::unsent_data>::at_schedule
                    (
                            [data=_data,_pxy=std::forward<pump::net::unsent_data>(pxy)](std::shared_ptr<reactor::flow_implent<pump::net::unsent_data>> f)mutable{
                                data->schedule
                                        (
                                                std::forward<pump::net::unsent_data>(_pxy),
                                                [f](FLOW_ARG(pump::net::unsent_data)&& v){
                                                    f->trigge(std::forward<FLOW_ARG(pump::net::unsent_data)>(v));
                                                }
                                        );
                            },
                            runner
                    );
        }
        ALWAYS_INLINE auto
        send_packet(const char* buf,size_t len, std::shared_ptr<reactor::flow_runner>& runner=reactor::_sp_immediate_runner_){
            return send_packet(pump::net::unsent_data(buf,len),runner)
                    .then([](FLOW_ARG(pump::net::unsent_data)&& v){
                        ____forward_flow_monostate_exception(v);
                        auto&& d=std::get<pump::net::unsent_data>(v);
                        return std::make_tuple(d.buf(),d.has_sent());
                    });
        }

    };
}
#endif //PROJECT_TCP_SESSION_HH
