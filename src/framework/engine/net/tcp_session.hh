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
#include <engine/reactor/flow.hh>
#include <engine/reactor/schdule.hh>
#include <engine/timer/timer_set.hh>

namespace engine::net{
    using namespace std::placeholders;
    namespace er=engine::reactor;
    struct send_proxy:boost::noncopyable{
        struct data{
            char*   _buf;
            size_t  _len;
            size_t  _cur;
        };
        std::shared_ptr<data> _data;
        send_proxy()= delete;
        explicit
        send_proxy(char* buf,size_t len):_data(std::make_shared<data>()){
            _data->_cur=0;
            _data->_buf=buf;
            _data->_len=len;
        }
        explicit
        send_proxy(std::shared_ptr<data>& d){
            _data=d;
        }
        send_proxy(const send_proxy&& o)noexcept{
            _data=o._data;
        }
        send_proxy(send_proxy&& o)noexcept{
            _data=o._data;
        }
        send_proxy& operator=(send_proxy&& x) noexcept {
            if (this != &x) {
                this->_data=x._data;
            }
            return *this;
        }
        const char*
        head(){
            return _data->_buf+_data->_cur;
        }
        size_t
        send_len(){
            return _data->_len>_data->_cur?_data->_len-_data->_cur:0;
        }
        bool
        done(){
            return _data->_len<=_data->_cur;
        }
        void
        update(size_t t){
            _data->_cur+=t;
        }
        template <typename _F_>
        auto
        send(_F_&& f){
            int l=f(_data->_buf,_data->_len-_data->_cur);
            if(l>0)
                _data->_cur+=l;
            return l;
        }
    };
    struct tcp_session:boost::noncopyable{
        using recv_sch_args_t=FLOW_ARG(common::ringbuffer*);
        using send_sch_args_t=FLOW_ARG(send_proxy);
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
            std::list<std::pair<send_proxy,send_sch_func_t>> waiting_send_tasks;
            common::ringbuffer _recv_buf;

            data():state(0),kd(0),fd(0),kevset(),cb(){}

            void
            schedule(recv_handler&& f){
                waiting_recv_tasks.emplace_back(std::forward<recv_sch_func_t>(f));
            }

            void
            schedule(send_proxy&& a,send_sch_func_t&& f){
                waiting_send_tasks.emplace_back(std::make_pair(std::forward<send_proxy>(a),std::forward<send_sch_func_t>(f)));
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
            last_will(){
            }
            void
            on_send_event(){
                while (!waiting_send_tasks.empty()){
                    auto&& [a,f]=waiting_send_tasks.front();
                    ssize_t n=ff_write(fd,a.head(),a.send_len());
                    if(n>0){
                        a.update(n);
                        if(a.done()){
                            f(send_sch_args_t(std::forward<send_proxy>(a)));
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
                last_will();
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
        wait_packet(int ms,size_t len=0){
            if(_data->_recv_buf.empty() || _data->_recv_buf.size()<len){
                using f_type=common::ncpy_func<void(FLOW_ARG(std::variant<int,common::ringbuffer*>))>;
                return er::flow_builder<std::variant<int,common::ringbuffer*>>::at_schedule
                        (
                                [data=_data,ms,len](std::shared_ptr<er::flow_implent<std::variant<int,common::ringbuffer*>>> sp_flow){
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
                                    engine::timer::_sp_timer_set->add_timer(ms,[ms,sp_flow](){
                                        sp_flow->trigge(FLOW_ARG(std::variant<int,common::ringbuffer*>)(
                                                std::variant<int,common::ringbuffer*>(ms)));
                                    });
                                },
                                er::_sp_immediate_runner_
                        );
            }
            else{
                return er::make_imme_flow()
                        .then([_data=this->_data](FLOW_ARG()&& a){
                            return std::variant<int,common::ringbuffer*>(&_data->_recv_buf);
                        });
            }
        }
        ALWAYS_INLINE reactor::flow_builder<send_proxy>
        send_packet(send_proxy&& pxy){
            return er::flow_builder<send_proxy>::at_schedule
                    (
                            [data=_data,_pxy=std::forward<send_proxy>(pxy)](std::shared_ptr<er::flow_implent<send_proxy>> f)mutable{
                                data->schedule
                                        (
                                                std::forward<send_proxy>(_pxy),
                                                [f](FLOW_ARG(send_proxy)&& v){
                                                    f->trigge(std::forward<FLOW_ARG(send_proxy)>(v));
                                                }
                                        );
                            },
                            er::_sp_immediate_runner_
                    );
        }
        ALWAYS_INLINE auto
        send_packet(char* buf,size_t len){
            return send_packet(send_proxy(buf,len));
        }
    };
}
#endif //PROJECT_TCP_SESSION_HH
