# pump

----目前开发中,不能应用于生产环境

----超高并发C++服务器无锁框架（PUMP）。以及基于框架开发的消息队列系统（APP）（开发中）
----目前已经实现基于DPDK(f-stack,freeBSD的TCP栈)的高速网络模块

        基于C++17实现的flow异步处理框架.
        
        目前框架+CACHE在未细化调优参数的情况下的能力:
        
        I78086K.12CPU核心,每个核心并发10000个异步任务(12万并发)
        
        每个任务分5个flow向CACHE插入/替换25条记录,.循环2000次
        
        一共插入/替换6亿条数据,CPU满载下完成时间21.073秒.
        
        每秒插入28472452条记录
        
        每秒完成任务单元(flow)5694490条
        
        为了适应DPDK,项目改为多进程方式实现,基于DPDK的实现源码放入engine下,
        原PUMP目录下是基于传统epoll的代码

---目前的开发是基于SPDK实现用户态高速NVME硬盘下对象存储
---例子
```cpp
void 
echo_proc(engine::net::tcp_session&& session,int timeout){
    session.wait_packet(timeout)
            .to_schedule(engine::reactor::_sp_global_task_center_)
            .then([s=std::forward<engine::net::tcp_session>(session),timeout](FLOW_ARG(std::variant<int,common::ringbuffer*>)&& v)mutable{
                ____forward_flow_monostate_exception(v);
                std::variant<int,common::ringbuffer*> d=std::get<2>(v);
                switch(d.index()){
                    case 0:
                    {
                        std::cout<<"time out"<<std::endl;
                        char* sz=new char[10];
                        sprintf(sz,"time out");
                        return s.send_packet(sz,10);
                    }
                    case 1:
                    {
                        common::ringbuffer* data=std::get<common::ringbuffer*>(d);
                        size_t l=data->size();
                        char* sz=new char[l];
                        memcpy(sz,data->read_head(),l);
                        data->consume(l);
                        return s.send_packet(sz,l);
                    }
                }
            })
            .then([s=std::forward<engine::net::tcp_session>(session),timeout](FLOW_ARG(std::tuple<const char*,size_t>)&& v)mutable{
                switch (v.index()){
                    case 0:
                        std::cout<<"unknown exception."<<std::endl;
                        s._data->close();
                        return;
                    case 1:
                        try{
                            s._data->close();
                            std::rethrow_exception(std::get<std::exception_ptr>(v));
                        }
                        catch (std::exception& e){
                            std::cout<<e.what()<<std::endl;
                        }
                        catch (...){
                            std::cout<<"unknown exception."<<std::endl;
                        }
                        return;
                    default:
                        auto [c,l]=std::get<std::tuple<const char*,size_t>>(v);
                        delete[] c;
                        echo_proc(std::forward<engine::net::tcp_session>(s),timeout);
                }
            })
            .submit();
}

void
wait_connect_proc(std::shared_ptr<engine::net::tcp_listener> l){
    engine::net::wait_connect(l)
            .to_schedule(engine::reactor::_sp_global_task_center_)
            .then([l](FLOW_ARG(engine::net::tcp_session)&& v){
                ____forward_flow_monostate_exception(v);
                std::cout<<"new connection"<<std::endl;
                auto&& s=std::get<engine::net::tcp_session>(v);
                echo_proc(std::forward<engine::net::tcp_session>(std::get<engine::net::tcp_session>(v)),20000);
            })
            .then([l](FLOW_ARG()&& v){
                switch (v.index()){
                    case 0:
                        wait_connect_proc(l);
                        return;
                    default:
                        try{
                            std::rethrow_exception(std::get<std::exception_ptr>(v));
                        }
                        catch (std::exception& e){
                            std::cout<<e.what()<<std::endl;
                        }
                        catch (...){
                            std::cout<<"unknown exception."<<std::endl;
                        }
                }
            })
            .submit();
}

int 
main(int argc, char * argv[]){
    engine::wait_engine_initialled(argc,argv)
            .then([](FLOW_ARG(std::shared_ptr<engine::glb_context>)&& a){
                ____forward_flow_monostate_exception(a);
                std::cout<<"inited"<<std::endl;
                std::shared_ptr<engine::glb_context> _c=std::get<std::shared_ptr<engine::glb_context>>(a);
                wait_connect_proc(engine::net::start_tcp_listen(_c->kqfd, 9022));
                return _c;
            })
            .then([](FLOW_ARG(std::shared_ptr<engine::glb_context>)&& a){
                ____forward_flow_monostate_exception(a);
                engine::engine_run(
                        std::get<std::shared_ptr<engine::glb_context>>(a),
                        [](){}
                );
            })
            .then([](FLOW_ARG()&& a){
                switch(a.index()){
                    case 0:
                        std::cout<<"std::monostate"<<std::endl;
                    default:
                        try {
                            std::rethrow_exception(std::get<std::exception_ptr>(a));
                        }
                        catch (std::exception& e){
                            std::cout<<e.what()<<std::endl;
                        }
                        catch (...){
                            std::cout<<"unkonwn exception ... "<<std::endl;
                        }
                }
            })
            .submit();
}
```
