//////////////////////////////////////////////////////////////////////////
// File: threadpool.h
// Description: Manage, and run parallel tasks by threadpool
// Author: Le Xuan Tuan Anh
//
// Copyright 2022 Le Xuan Tuan Anh
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//////////////////////////////////////////////////////////////////////////

#ifndef THREAD_POOL_H__
#define THREAD_POOL_H__

#include "noncopyable.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

#define THREAD_POOl_DEFAULT_POOL_SIZE 2

#ifdef TP_CONSOLE
extern std::mutex _tpMtCout;
#define _tpLockPrint(text)                         \
    {                                              \
        std::lock_guard<std::mutex> lk(_tpMtCout); \
        std::cout << ">" << text << std::endl;     \
    }
#else
#define _tpLockPrint(text)
#endif

namespace athread
{

enum WorkerStatus
{
    NOAVAILABLE,
    WAITTING_TASK,
    WAITTING_START,
    BUSY,
    END
};

class ThreadPool;
class Worker;

/**
 * @brief The interface of a runnable.
 */
class Runnable
{
    friend class athread::Worker;

public:
    Runnable(){};
    virtual ~Runnable(){};

protected:
    virtual void run() = 0;
};

class PoolQueue : public std::queue<Runnable*>
{
    friend class ThreadPool;

public:
    virtual ~PoolQueue(){};

protected:
    PoolQueue();
};

class Worker
{
    friend class ThreadPool;

public:
    virtual ~Worker(){};

protected:
    Worker(std::uint32_t id);
    virtual int work(ThreadPool& pool, PoolQueue& queue);
    virtual int workFor(const std::chrono::milliseconds& aliveTime, ThreadPool& pool, PoolQueue& queue);

    std::atomic_int status;
    std::uint32_t id;

    void waitForStartSignal(ThreadPool& pool);
};

/**
 * @brief Thread pool is a base task distributor for multithreads, which limits the number of
 * threads created but still completes the task safely.
 * When thread pool receives a task, it consider if there are any idle workers to assign the task,
 * if not, a new worker will be created.
 * After completing the task, the worker will go to idle status to wait for the next task.
 * These workers persist until the thread pool receives the termination signal.
 * If the number of workers is equal to coreSize, the next new workers created will have aliveTime
 * applied (seasonal-worker).
 * If a seasonal-worker is idle for aliveTime, it's destroyed.
 * The total number of workers and seasonal-workers will not exceed maxSize.
 *
 */
class ThreadPool : public athread::noncopyable_::noncopyable
{

    friend Worker;

    class RunnableHolder : public Runnable
    {
    public:
        RunnableHolder(std::function<void()> f);

    protected:
        virtual void run() { fn(); };
        std::function<void()> fn;
    };

public:
    ThreadPool(std::uint32_t coreSize, int maxSize = std::thread::hardware_concurrency(),
               const std::chrono::milliseconds& aliveTime = std::chrono::seconds(60), bool waitForSignalStart = false);

    virtual ~ThreadPool();

    /**
     * @brief Check idle of all workers.
     * @return true if all workers  waiting for a new task, false if has any-workers doing.
     */
    virtual bool isIdle();

    /**
     * @brief Add a task to thread pool.
     * @param runnable the task to add
     * @return false if the thread pool was exited, otherwise true.
     */
    virtual bool push(Runnable* runnable);

    virtual bool push(const std::function<void()>& f);

    /**
     * @brief Add a task to thread pool. The task is constructed through forwarding arguments.
     * @tparam _Runnable the implement type of IRunnable.
     * @tparam Args the package type of forwarding arguments.
     * @param args arguments to forward to the constructor of the implement runnable.
     */
    template <typename _Runnable, class... Args>
    bool emplace(Args&&... args);

    /**
     * @brief Signals notifying the thread pool to start executing tasks in the queue.
     * This is not necessary if m_tpWaitForSignalStart was passed the true value in constructor.
     */
    virtual void start();

    /**
     * @brief Wait for workers complete tasks. This is safe to make sure all threads have been
     * exited.
     */
    virtual void wait();

    /**
     * @brief Signals notifying the thread pool to stop executing the remaining tasks. The executing
     * tasks will continue until they actually finish.
     * This is also called implicitly in deconstructor.
     * @param alsoWait is also to call wait().
     */
    virtual void terminate(bool alsoWait = true);

    /**
     * @brief Check executable of the thread pool for a new task.
     * @return true if possible, otherwise false.
     */
    virtual bool executable();

    /**
     * @brief detach all current threads.
     */
    virtual void detach();

    void cleanCompleteWorkers();

protected:
    ThreadPool() = default;
    virtual void createWorker(std::uint32_t count);
    virtual void createSeasonalWorker(std::uint32_t count,
                                      const std::chrono::milliseconds& aliveTime);

    std::uint32_t m_coreSize;
    int m_maxSize;
    std::chrono::milliseconds m_aliveTime;
    PoolQueue m_taskQueue;
    std::vector<std::unique_ptr<std::thread>> m_threads;
    std::vector<std::unique_ptr<athread::Worker>> m_workers;

    std::mutex m_queueLocker;
    std::condition_variable m_cv;
    std::atomic_bool m_terminalSignal{false};
    std::atomic_bool m_waitForSignalStart{false};
};

/**
 * @brief ThreadPoolFixed is the same with ThreadPool.
 * However,
 * All tasks are executed only after the start signal is given.
 * The thread pool will exit automatically when there are no more tasks in the queue or the
 * terminate signal is given.
 * The new tasks are still pushable and executable after the start signal is given, as long as the
 * thread pool is still not exited.
 */
class ThreadPoolFixed : public ThreadPool
{
public:
    explicit ThreadPoolFixed(std::uint32_t coreSize);
    ~ThreadPoolFixed();

    /**
     * @brief Check executable of the thread pool for a new task.
     * @return false if the terminate signal is given or the thread pool is exited, otherwise true.
     */
    virtual bool executable();

protected:
    ThreadPoolFixed() = default;
    virtual void createWorker(std::uint32_t count);
};

template <typename _Runnable, class... Args>
bool ThreadPool::emplace(Args&&... args)
{
    return push(new _Runnable(std::forward<Args>(args)...));
}

} // namespace athread

#endif
