#include "threadpool.h"

int threadpool::worker(threadpool::IThreadPool* pool, threadpool::PoolQueue* queue)
{
    std::shared_ptr<threadpool::IRunnable> backElm = nullptr;
    do
    {
        {
            std::unique_lock lk{pool->_tpMtQueue};

            pool->_tpCV.wait(
                lk,
                [&]()
                {
                    return !pool->_tpWaitForSignalStart.load(std::memory_order_relaxed) &&
                           (pool->_tpTerminal.load(std::memory_order_relaxed) || !queue->empty());
                });

            if (pool->_tpTerminal.load(std::memory_order_relaxed)) break;

            // get back element
            backElm = queue->front();
            queue->pop();

            lk.unlock();
        }

        if (backElm) backElm->run();
        backElm = nullptr;

    } while (true);

    return 1;
}

int threadpool::seasonalWorker(threadpool::IThreadPool* pool, threadpool::PoolQueue* queue,
                               const std::chrono::nanoseconds& aliveTime)
{
    std::shared_ptr<threadpool::IRunnable> backElm = nullptr;
    do
    {
        {
            std::unique_lock lk{pool->_tpMtQueue};
            if (!pool->_tpCV.wait_for(
                    lk, aliveTime,
                    [&]()
                    {
                        return !pool->_tpWaitForSignalStart.load(std::memory_order_relaxed) &&
                               (pool->_tpTerminal.load(std::memory_order_relaxed) ||
                                !queue->empty());
                    }))
            {
                break;
            }

            if (pool->_tpTerminal.load(std::memory_order_relaxed)) break;

            // get back element
            backElm = queue->front();
            queue->pop();

            lk.unlock();
        }

        if (backElm) backElm->run();
        backElm = nullptr;

    } while (true);

    return 1;
}

threadpool::PoolQueue::PoolQueue() {}

threadpool::ThreadPool::ThreadPool(PoolQueue& queue,
                                   std::uint32_t poolSize /*= THREAD_POOl_DEFAULT_POOL_SIZE*/,
                                   bool waitForSignalStart /*= false*/)
{
    m_poolSize = poolSize;
    m_taskQueue.swap(queue);
    _tpWaitForSignalStart.store(waitForSignalStart);
    createWorker(m_poolSize);
}

threadpool::ThreadPool::ThreadPool(std::uint32_t poolSize /*= THREAD_POOl_DEFAULT_POOL_SIZE*/,
                                   bool waitForSignalStart /*= false*/)
    : ThreadPool(m_taskQueue, poolSize, waitForSignalStart)
{
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

void threadpool::ThreadPool::createWorker(std::uint32_t count)
{
    for (std::uint32_t i = 0; i < count; i++)
        m_threads.push_back(std::make_unique<std::thread>(worker, this, &m_taskQueue));
}
