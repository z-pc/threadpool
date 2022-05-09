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

class IThreadPool;
int worker(threadpool::IThreadPool* pool, threadpool::PoolQueue* queue, std::string name);

class IThreadPool : public boost::noncopyable_::noncopyable
{
public:
    IThreadPool() { m_poolSize = 0; };
    virtual ~IThreadPool(){};

    virtual void execute(std::shared_ptr<IRunnable> runnable) = 0;
    virtual void start() = 0;
    virtual void wait() = 0;
    virtual void terminate() = 0;

    friend int worker(threadpool::IThreadPool* pool, threadpool::PoolQueue* queue, std::string name);

protected:
    std::uint32_t m_poolSize;
    PoolQueue m_taskQueue;
    std::vector<std::unique_ptr<std::thread>> m_threads;

    std::mutex _tpMtQueue;
    std::mutex _tpMtCoutStream;
    std::condition_variable _tpCV;
    std::atomic_bool _tpQueueReady = false;
    std::atomic_bool _tpTerminal = false;
    std::atomic_bool _tpWaitForSignalStart = false;
};

class ThreadPool : public IThreadPool
{

public:
    ThreadPool(std::uint32_t poolSize, bool waitForSignalStart = false);
    ThreadPool(std::uint32_t poolSize, PoolQueue& queue, bool waitForSignalStart = false);
    virtual ~ThreadPool();

    void execute(std::shared_ptr<IRunnable> runnable);
    void start();
    void wait();
    void terminate();

protected:
    ThreadPool();
    virtual void createThreads(std::uint32_t count);
};

class ThreadPoolDynamic : public ThreadPool
{
public:
    template <class Rep, class Period>
    ThreadPoolDynamic(std::uint32_t poolSize, std::uint32_t poolMaxSize,
                      const std::chrono::duration<Rep, Period>& aliveTime, PoolQueue& queue,
                      bool waitForSignalStart = false);
    ~ThreadPoolDynamic(){};

protected:
    virtual void createThreads(std::uint32_t count) override;

    std::uint32_t m_poolMaxSize;
    std::chrono::nanoseconds m_aliveTime;
};

template <class Rep, class Period>
threadpool::ThreadPoolDynamic::ThreadPoolDynamic(
    std::uint32_t poolSize, std::uint32_t poolMaxSize,
    const std::chrono::duration<Rep, Period>& aliveTime, PoolQueue& queue,
    bool waitForSignalStart /*= false*/)
//: ThreadPool(poolSize, queue, waitForSignalStart)
{
    extern std::atomic_bool _tpWaitForSignalStart;
    m_poolSize = poolSize;
    m_taskQueue.swap(queue);
    _tpWaitForSignalStart.store(waitForSignalStart);
    createThreads(m_poolSize);
    m_aliveTime = aliveTime;
    m_poolMaxSize = poolMaxSize;
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
