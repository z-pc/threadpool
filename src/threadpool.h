#ifndef THREAD_POOL_H__
#define THREAD_POOL_H__

#include "noncopyable.h"
#include <future>
#include <iostream>
#include <queue>

namespace threadpool
{

class IRunnable
{

public:
    IRunnable(){};
    virtual ~IRunnable(){};

    virtual bool run() = 0;
};

class PoolQueue : public std::queue<std::shared_ptr<IRunnable>>
{
public:
    PoolQueue();
    virtual ~PoolQueue(){};
};

class ThreadPool : public boost::noncopyable_::noncopyable
{

public:
    ThreadPool(std::uint32_t poolSize, std::uint32_t maxPoolSize, PoolQueue& queue,
               std::uint64_t keepAliveTime, bool waitForSignalStart);

    virtual ~ThreadPool(){};

    template <class Runnable_> void execute(const Runnable_& runnable);
    void execute(std::shared_ptr<IRunnable> runnable);
    void start();
    void wait();
    void terminate();

protected:
    ThreadPool();
    void createThreads(std::uint32_t count);

    std::uint32_t m_poolSize;
    std::uint32_t m_maxPoolSize;
    PoolQueue m_taskQueue;

    std::vector<std::unique_ptr<std::thread>> m_threads;
};

template <class Runnable_> void threadpool::ThreadPool::execute(const Runnable_& runnable)
{
    std::shared_ptr<IRunnable> r = std::make_shared<Runnable_>(runnable);
    execute(r);
}

} // namespace threadpool

class RunnableExample : public threadpool::IRunnable
{
public:
    RunnableExample(std::string name) { m_name = name; };
    ~RunnableExample(){};

    virtual bool run() override
    {

        using namespace std::chrono_literals;

        for (int i = 0; i < 5; i++)
        {
            std::cout << m_name << " running " << i << std::endl;
            std::this_thread::sleep_for(1s);
        }

        return true;
    }

    std::string m_name;
};
#endif
