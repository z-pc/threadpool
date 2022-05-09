#include "threadpool.h"
#include <atomic>
#include <iostream>
#include <sstream>

int threadpool::worker(threadpool::IThreadPool* pool, threadpool::PoolQueue* queue,
                       std::string name)
{
    std::string workerName = name;
    std::shared_ptr<threadpool::IRunnable> backElm = nullptr;
    {
        const std::lock_guard<std::mutex> lk(pool->_tpMtCoutStream);
        std::cout << workerName << " has been created" << std::endl;
    }

    do
    {
        {
            std::unique_lock lk{pool->_tpMtQueue};
            {
                const std::lock_guard<std::mutex> lk(pool->_tpMtCoutStream);
                std::cout << workerName << " waitting" << std::endl;
            }
            pool->_tpCV.wait(
                lk,
                [&]()
                {
                    return !pool->_tpWaitForSignalStart.load(std::memory_order_relaxed) &&
                           (pool->_tpTerminal.load(std::memory_order_relaxed) || !queue->empty());
                });

            if (pool->_tpTerminal.load(std::memory_order_relaxed))
            {
                {
                    const std::lock_guard<std::mutex> lk(pool->_tpMtCoutStream);
                    std::cout << workerName << "is terminal" << std::endl;
                }
                break;
            }
            {
                const std::lock_guard<std::mutex> lk(pool->_tpMtCoutStream);
                std::cout << workerName << " wait done" << std::endl;
            }
            // get back element
            backElm = queue->front();
            queue->pop();

            lk.unlock();
        }

        // do somethings
        {
            const std::lock_guard<std::mutex> lk(pool->_tpMtCoutStream);
            std::string t = ((RunnableExample*)(backElm.get()))->m_name;
            std::cout << workerName << " do " << t << std::endl;
        }

        if (backElm) backElm->run();
        backElm = nullptr;

    } while (true);

    return 1;
}

template <class Rep, class Period>
int seasonalWorker(threadpool::PoolQueue* queue,
                   const std::chrono::duration<Rep, Period>& aliveTime, std::string name)
{
    std::string workerName = name;
    std::shared_ptr<threadpool::IRunnable> backElm = nullptr;
    std::cout << workerName << "seasonal has been created" << std::endl;

    do
    {
        {
            std::unique_lock lk{_tpMtQueue};
            std::cout << workerName << "seasonal waitting" << std::endl;
            if (!_tpCV.wait_for(lk, aliveTime,
                                [&]()
                                {
                                    return !_tpWaitForSignalStart.load(std::memory_order_relaxed) &&
                                           (_tpTerminal.load(std::memory_order_relaxed) ||
                                            !queue->empty());
                                }))
            {
                std::cout << workerName << "seasonal is terminal" << std::endl;
                break;
            }

            std::cout << workerName << " seasonal wait done" << std::endl;
            // get back element
            backElm = queue->front();
            queue->pop();

            lk.unlock();
        }

        // do somethings
        {
            std::string t = ((RunnableExample*)(backElm.get()))->m_name;
            std::cout << workerName << "seasonal do " << t << std::endl;
        }

        if (backElm) backElm->run();
        backElm = nullptr;

    } while (true);

    return 1;
}

threadpool::PoolQueue::PoolQueue() {}

threadpool::ThreadPool::ThreadPool() { m_poolSize = 2; }

threadpool::ThreadPool::ThreadPool(std::uint32_t poolSize, bool waitForSignalStart /*= false*/)
    : ThreadPool(poolSize, m_taskQueue, waitForSignalStart)
{
}

threadpool::ThreadPool::ThreadPool(std::uint32_t poolSize, PoolQueue& queue,
                                   bool waitForSignalStart /*= false*/)
{
    m_poolSize = poolSize;
    m_taskQueue.swap(queue);
    _tpWaitForSignalStart.store(waitForSignalStart);
    createThreads(m_poolSize);
}

threadpool::ThreadPool::~ThreadPool() { terminate(); }

void threadpool::ThreadPool::execute(std::shared_ptr<IRunnable> runnable)
{
    {
        std::lock_guard lk{_tpMtQueue};
        m_taskQueue.push(runnable);
        _tpCV.notify_one();
    }
}

void threadpool::ThreadPool::start()
{
    _tpWaitForSignalStart.store(false);
    _tpCV.notify_all();
}

void threadpool::ThreadPool::wait()
{
    for (auto& th : m_threads)
        if (th->joinable()) th->join();
}

void threadpool::ThreadPool::terminate()
{
    _tpTerminal.store(true);
    _tpCV.notify_all();
}

void threadpool::ThreadPool::createThreads(std::uint32_t count)
{
    for (std::uint32_t i = 0; i < count; i++)
    {
        std::stringstream sstr;
        sstr << "#w " << i << " #";
        std::string name = sstr.str();
        m_threads.push_back(std::make_unique<std::thread>(worker, this, &m_taskQueue, name));
    }
}

void threadpool::ThreadPoolDynamic::createThreads(std::uint32_t count)
{
    for (std::uint32_t i = 0; i < count; i++)
    {
        std::stringstream sstr;
        sstr << "#wddd" << i << "#";
        std::string name = sstr.str();
        m_threads.push_back(std::make_unique<std::thread>(worker, this, &m_taskQueue, name));
    }
}
