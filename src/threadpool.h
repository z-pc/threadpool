#ifndef THREAD_POOL_H__
#define THREAD_POOL_H__

#include <future>
#include <queue>

namespace threadpool
{

class IRunnable
{

public:
    IRunnable(){};
    virtual ~IRunnable(){};

    virtual std::future<bool> run() = 0;
};

class PoolQueue : public std::queue<IRunnable>
{
public:
    PoolQueue();
    virtual ~PoolQueue(){};
};

class ThreadPool
{

public:
    ThreadPool(std::uint32_t poolSize, std::uint32_t maxPoolSize, const PoolQueue& queue,
               std::uint32_t keepAliveTime, bool waitForStartNotify);
    virtual ~ThreadPool(){};

    void execute(const IRunnable& runnable);
    void start();
    std::future<bool> wait();
    void terminate();

protected:
    std::uint32_t m_poolSize;
    std::uint32_t m_maxPoolSize;
    PoolQueue m_taskQueue;
};

} // namespace threadpool

#endif
