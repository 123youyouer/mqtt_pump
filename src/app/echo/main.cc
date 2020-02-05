//
// Created by null on 20-2-1.
//

#include <net/tcp_session.hh>
#include <pump.hh>
#include <utils/lru_cache.hh>
#include <data/unaligned_cast.hh>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "InfiniteRecursion"

void
wait_ping(net::tcp_session& session){
    session.wait_packet()
            .then([&session](net::linear_ringbuffer_st *buf)mutable{
                session.send_data(buf->read_head(),buf->size())
                        .then([&session,buf,len=buf->size()](){
                            buf->consume(len);
                            wait_ping(session);
                        })
                        .submit();
            })
            .when_exception([&session](std::exception_ptr p){
                try{
                    if(p)
                        std::rethrow_exception(p);
                }
                catch (const net::tcp_session_exception& e){
                    logger::info("tcp session exception :{0} session closed",e.code);
                }
                catch (const std::exception& e){
                    std::cout<<e.what()<<std::endl;
                }
                catch (...){
                    logger::info("cant catch exception in flow::when_exception");
                }
                return false;
            })
            .submit();
}

template<uint16_t _PORT_>
void run_echo(net::listener<_PORT_>& l){
    net::wait_connect(l)
            .then([&l](net::session_data &&session_d) {
                net::tcp_session *s = new net::tcp_session(session_d);
                wait_ping(s->start_at(hw::cpu_core::_01));
                run_echo(l);
            })
            .submit();
}
#pragma clang diagnostic pop

#include <boost/intrusive/list.hpp>
#include <data/cahce.hh>
class tag_1{};
using lm=boost::intrusive::link_mode<boost::intrusive::auto_unlink>;
using BaseHook = boost::intrusive::list_base_hook<lm>;
class adata : public BaseHook{
public:
    int a;
    explicit adata(int _a):a(_a){}
};
typedef boost::intrusive::list<adata, boost::intrusive::base_hook<BaseHook>,boost::intrusive::constant_time_size<false>> FooList;
int main(){
    char sz[]={0x01,0x03,0x00,0x05};

    std::string_view v(sz);
    std::cout<<v.size()<<std::endl;
    int a=v.length();


    data::cache_lsu<int> cache;

    cache.push("1",std::make_shared<int>(1));

    using _t_=int;

    auto d=cache.find<_t_>("1");

    std::cout<<*std::get<std::shared_ptr<_t_>>(d)<<std::endl;

    *std::get<std::shared_ptr<_t_>>(d)=9;

    d=cache.find<int>("1");

    std::cout<<*std::get<std::shared_ptr<_t_>>(d)<<std::endl;

    cache.push("2",std::make_shared<int>(2));

    FooList l;
    adata a2(2);
    l.push_back(a2);
    {
        adata a1(1);
        l.push_back(a1);
        a1.a=3;
    }

    std::cout<<l.size()<<":"<<l.begin()->a<<std::endl;


    hw::pin_this_thread(0);
    sleep(1);
    hw::the_cpu_count=1;
    engine::init_engine
            <
                    reactor::sortable_task<utils::noncopyable_function<void()>,1>,
                    reactor::sortable_task<utils::noncopyable_function<void()>,2>
            >();
    sleep(1);

    utils::lru_cache<std::string,std::string> a_cache;

    logger::info("echo server started ......");
    run_echo(net::listener<9022>::instance.start_at(hw::cpu_core::_01));
    logger::info("echo server listen at {}",9022);
    sleep(10000);
    return 0;
}