#ifndef THREAD_POOL_H__
#define THREAD_POOL_H__

#include "noncopyable.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

#define THREAD_POOl_DEFAULT_POOL_SIZE 2

#ifdef TP_CONSOLE
extern std::mutex _tpMtCout;
#define tpLockPrint(text)                                                                          \
    {                                                                                              \
        std::lock_guard lk(_tpMtCout);                                                             \
        std::cout << ">" << text << std::endl;                                                     \
    }
#else
#define tpLockPrint(text)
#endif

namespace threadpool
{

using namespace std::literals::chrono_literals;

enum WorkerStatus
{
    NOAVAILABLE,
    WAITTING,
    BUSY,
    END
};

class IThreadPool;
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

struct Worker
{
    Worker(int id);
    virtual int work(IThreadPool* pool, PoolQueue* queue);
    virtual int workFor(const std::chrono::nanoseconds& aliveTime, IThreadPool* pool,
                        PoolQueue* queue);
    std::atomic_int status;
    int id;
};

class IThreadPool : public boost::noncopyable_::noncopyable
{
    friend Worker;

public:
    IThreadPool()
    {
        m_poolSize = 0;
        m_poolMaxSize = 0;
        m_aliveTime = 0s;
    };
    virtual ~IThreadPool(){};

    virtual void push(std::shared_ptr<IRunnable> runnable) = 0;
    virtual void start() = 0;
    virtual void wait() = 0;
    virtual void terminate() = 0;

protected:
    std::uint32_t m_poolSize;
    std::uint32_t m_poolMaxSize;
    std::chrono::nanoseconds m_aliveTime;
    PoolQueue m_taskQueue;
    std::vector<std::unique_ptr<std::thread>> m_threads;
    std::vector<std::unique_ptr<threadpool::Worker>> m_workers;

    std::mutex m_mtQueue;
    std::mutex m_mtThreads;
    std::condition_variable m_tpCV;
    std::atomic_bool m_tpTerminal = false;
    std::atomic_bool m_tpWaitForSignalStart = false;
};

class ThreadPool : public IThreadPool
{
public:
    ThreadPool(std::uint32_t poolSize = THREAD_POOl_DEFAULT_POOL_SIZE,
               std::uint32_t poolMaxSize = std::thread::hardware_concurrency(),
               const std::chrono::nanoseconds& aliveTime = 60s, bool waitForSignalStart = false);
    virtual ~ThreadPool();

    virtual void push(std::shared_ptr<IRunnable> runnable);
    template <typename _Runnable, class... Args> void emplace(Args&&... args);
    void start();
    void wait();
    void terminate();

protected:
    virtual void createWorker(std::uint32_t count);
    virtual void createSeasonalWorker(std::uint32_t count,
                                      const std::chrono::nanoseconds& aliveTime);
    void cleanCompleteWorker();
    bool cleanBackCompleteWorker();
};

template <typename _Runnable, class... Args> void threadpool::ThreadPool::emplace(Args&&... args)
{
    std::shared_ptr<IRunnable> r = std::make_shared<_Runnable>(std::forward<Args>(args)...);
    push(r);
}

} // namespace threadpool

#endif
