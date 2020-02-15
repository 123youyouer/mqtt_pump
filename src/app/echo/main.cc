//
// Created by null on 20-2-1.
//
#include <boost/program_options.hpp>
#include <pump/net/tcp_session.hh>
#include <pump/logger/logger.hh>
#include <pump/utils/lru_cache.hh>
#include <pump/data/unaligned_cast.hh>
#include <pump/timer/timer_set.hh>
#include <pump/pump.hh>
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
#include <pump/data/cahce.hh>
class tag_1{};
using lm=boost::intrusive::link_mode<boost::intrusive::auto_unlink>;
using BaseHook = boost::intrusive::list_base_hook<lm>;
class adata : public BaseHook{
public:
    int a;
    explicit adata(int _a):a(_a){}
};
typedef boost::intrusive::list<adata, boost::intrusive::base_hook<BaseHook>,boost::intrusive::constant_time_size<false>> FooList;
thread_local data::cache_lsu<int,std::string> cache_on_thread;

uint64_t t=0;

PUMP_INLINE void
test(int cpu,int count){
    reactor::at_cpu(hw::cpu_core(cpu))
            .then([](){
                cache_on_thread.push("1",std::make_shared<int>(1));
                cache_on_thread.push("2",std::make_shared<int>(2));
                cache_on_thread.push("3",std::make_shared<int>(3));
                cache_on_thread.push("4",std::make_shared<int>(4));
                cache_on_thread.push("5",std::make_shared<int>(5));
            })
            .then([](){
                cache_on_thread.push("1",std::make_shared<int>(1));
                cache_on_thread.push("2",std::make_shared<int>(2));
                cache_on_thread.push("3",std::make_shared<int>(3));
                cache_on_thread.push("4",std::make_shared<int>(4));
                cache_on_thread.push("5",std::make_shared<int>(5));
            })
            .then([](){
                cache_on_thread.push("1",std::make_shared<int>(1));
                cache_on_thread.push("2",std::make_shared<int>(2));
                cache_on_thread.push("3",std::make_shared<int>(3));
                cache_on_thread.push("4",std::make_shared<int>(4));
                cache_on_thread.push("5",std::make_shared<int>(5));
            })
            .then([](){
                cache_on_thread.push("1",std::make_shared<int>(1));
                cache_on_thread.push("2",std::make_shared<int>(2));
                cache_on_thread.push("3",std::make_shared<int>(3));
                cache_on_thread.push("4",std::make_shared<int>(4));
                cache_on_thread.push("5",std::make_shared<int>(5));
            })
            .then([](){
                cache_on_thread.push("1",std::make_shared<int>(1));
                cache_on_thread.push("2",std::make_shared<int>(2));
                cache_on_thread.push("3",std::make_shared<int>(3));
                cache_on_thread.push("4",std::make_shared<int>(4));
                cache_on_thread.push("5",std::make_shared<int>(5));
            })
            .then([cpu,i=count](){
                if((i)<2000){
                    test(cpu,i+1);
                }
                else{
                    std::cout<<"..."<<timer::now_tick()-t<<std::endl;
                }
            })
            .submit();
}

int main(int argc, char * argv[]){
    boost::program_options::options_description ops;

    std::string config;

    ops.add_options()
            ("cofig",boost::program_options::value<std::string>(&config)->default_value("config.ini"));

    boost::program_options::variables_map vm;

    try{
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, ops), vm);
    }
    catch(...){
        std::cout << "输入的参数中存在未定义的选项！\n";
        return 0;
    }


    hw::pin_this_thread(0);
    sleep(1);
    //hw::the_cpu_count=1;
    engine::init_engine
            <
                    reactor::sortable_task<utils::noncopyable_function<void()>,1>,
                    reactor::sortable_task<utils::noncopyable_function<void()>,2>
            >();
    sleep(1);


    t=timer::now_tick();
    std::cout<<t<<std::endl;
    for(int i=0;i<hw::the_cpu_count;++i){
        for(int j=0;j<1000;j++)
            test(i,0);
    }
    /*
    reactor::at_cpu(hw::cpu_core::_01)
            .then([](){
                cache_on_thread.push("1",std::make_shared<std::string>("2"));
                return 10;
            })
            .at_cpu(hw::cpu_core::_02)
            .then([](int&& i){
                cache_on_thread.push("1",std::make_shared<std::string>("3"));
            })
            .at_cpu(hw::cpu_core::_01)
            .then([](){
                auto x=(*std::get<2>(*(std::get<1>(cache_on_thread.find("1")))));
                logger::info("{}",x);
            })
            .at_cpu(hw::cpu_core::_02)
            .then([](){
                auto x=(*std::get<2>(*(std::get<1>(cache_on_thread.find("1")))));
                logger::info("{}",x);
            })
            .submit();

    logger::info("echo server started ......");
    run_echo(net::listener<9022>::instance.start_at(hw::cpu_core::_01));
    logger::info("echo server listen at {}",9022);
     */
    sleep(10000);
    return 0;
}