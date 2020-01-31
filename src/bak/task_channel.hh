//
// Created by null on 19-9-11.
//

#ifndef CPPMQ_TASK_CHANNEL_HH
#define CPPMQ_TASK_CHANNEL_HH
#include <boost/noncopyable.hpp>
#include "moodycamel/readerwriterqueue.h"
namespace reactor{
    namespace channel{
        template<typename T, size_t MAX_BLOCK_SIZE>
        using task_channel_fifo_type = moodycamel::ReaderWriterQueue<T,MAX_BLOCK_SIZE>;

        constexpr
        const unsigned int max_channel_row_col=128;

        template <class _TASK_TYPE>
        struct task_channel : public boost::noncopyable{
            std::unique_ptr<task_channel_fifo_type<_TASK_TYPE,1024>> _tasks;
            task_channel(task_channel&& other)noexcept
                    :_tasks(other._tasks.release())
            {}

        public:
            explicit task_channel()
                    :_tasks(std::make_unique<task_channel_fifo_type<_TASK_TYPE,1024>>())
            {};
            /*
            template <class T>
            friend constexpr void init(unsigned int cpu_core_count);
             */
        };

        template <class _TASK_TYPE>
        task_channel<_TASK_TYPE>* _all_channels[max_channel_row_col][max_channel_row_col];
    }

    template <class _TASK_TYPE>
    constexpr
    void init_all_channels(int cpu_core_count){
        for(unsigned int src=0;src<cpu_core_count;++src){
            for(unsigned int dst=0;dst<cpu_core_count;++dst){
                channel::_all_channels<_TASK_TYPE>[src][dst]=new channel::task_channel<_TASK_TYPE>();
            }
        }
    }
    template <class _TASK_TYPE0,class _TASK_TYPE1,class ..._TASK_TYPE2>
    constexpr
    void init_all_channels(unsigned int cpu_core_count){
        init_all_channels<_TASK_TYPE0>(cpu_core_count);
        init_all_channels<_TASK_TYPE1,_TASK_TYPE2...>(cpu_core_count);
    }
}
#endif //CPPMQ_TASK_CHANNEL_HH
