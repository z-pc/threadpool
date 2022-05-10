#ifndef THREAD_POOL_H__
#define THREAD_POOL_H__

#include "noncopyable.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <queue>
#include <thread>

#define THREAD_POOl_DEFAULT_POOL_SIZE 2

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
int worker(threadpool::IThreadPool* pool, threadpool::PoolQueue* queue);
int seasonalWorker(threadpool::IThreadPool* pool, threadpool::PoolQueue* queue,
                   const std::chrono::nanoseconds& aliveTime);

class IThreadPool : public boost::noncopyable_::noncopyable
{
public:
    IThreadPool() { m_poolSize = 0; };
    virtual ~IThreadPool(){};

    virtual void execute(std::shared_ptr<IRunnable> runnable) = 0;
    virtual void start() = 0;
    virtual void wait() = 0;
    virtual void terminate() = 0;

    friend int worker(threadpool::IThreadPool* pool, threadpool::PoolQueue* queue);
    friend int seasonalWorker(threadpool::IThreadPool* pool, threadpool::PoolQueue* queue,
                              const std::chrono::nanoseconds& aliveTime);

protected:
    std::uint32_t m_poolSize;
    PoolQueue m_taskQueue;
    std::vector<std::unique_ptr<std::thread>> m_threads;

    std::mutex _tpMtQueue;
    std::condition_variable _tpCV;
    std::atomic_bool _tpQueueReady = false;
    std::atomic_bool _tpTerminal = false;
    std::atomic_bool _tpWaitForSignalStart = false;
};

class ThreadPool : public IThreadPool
{
public:
    ThreadPool(std::uint32_t poolSize = THREAD_POOl_DEFAULT_POOL_SIZE,
               bool waitForSignalStart = false);
    ThreadPool(PoolQueue& queue, std::uint32_t poolSize = THREAD_POOl_DEFAULT_POOL_SIZE,
               bool waitForSignalStart = false);
    virtual ~ThreadPool();

    virtual void execute(std::shared_ptr<IRunnable> runnable);
    void start();
    void wait();
    void terminate();

protected:
    virtual void createWorker(std::uint32_t count);
};

} // namespace threadpool

#endif
