#include "threadpool.h"

threadpool::PoolQueue::PoolQueue() {}

threadpool::ThreadPool::ThreadPool(std::uint32_t poolSize, std::uint32_t maxPoolSize,
                                   const PoolQueue& queue, std::uint32_t keepAliveTime,
                                   bool waitForStartNotify)
{

}
