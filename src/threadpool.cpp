#include "threadpool.h"
#include <atomic>
#include <iostream>

std::mutex _tpMtQueue;
std::condition_variable _tpCV;
std::atomic_bool _tpQueueReady = false;
std::atomic_bool _tpTerminal = false;
std::atomic_bool _tpWaitForSignalStart = false;

int worker(threadpool::PoolQueue* queue, std::string name)
{
    std::string workerName = name;
    std::shared_ptr<threadpool::IRunnable> backElm = nullptr;
    std::cout << workerName << " has been created" << std::endl;

    do
    {
        {
            std::unique_lock lk{_tpMtQueue};
            std::cout << workerName << " waitting" << std::endl;
            _tpCV.wait(lk,
                       [&]()
                       {
                           return !_tpWaitForSignalStart.load(std::memory_order_relaxed) &&
                                  (_tpTerminal.load(std::memory_order_relaxed) || !queue->empty());
                       });

            if (_tpTerminal.load(std::memory_order_relaxed))
            {
                std::cout << workerName << "is terminal" << std::endl;
                break;
            }

            std::cout << workerName << " wait done" << std::endl;
            // get back element
            backElm = queue->front();
            queue->pop();

            lk.unlock();
        }

        // do somethings
        {
            std::string t = ((RunnableExample*)(backElm.get()))->m_name;
            std::cout << workerName << " do " << t << std::endl;
        }

        if (backElm) backElm->run();
        backElm = nullptr;

    } while (true);

    return 1;
}

threadpool::PoolQueue::PoolQueue() {}

threadpool::ThreadPool::ThreadPool()
{
    m_poolSize = 0;
    m_maxPoolSize = std::hardware_constructive_interference_size;
}

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

threadpool::ThreadPool::ThreadPool(std::uint32_t poolSize, std::uint32_t maxPoolSize,
                                   PoolQueue& queue, std::uint64_t keepAliveTime,
                                   bool waitForSignalStart)
{
    m_poolSize = poolSize;
    m_maxPoolSize = maxPoolSize;
    m_taskQueue.swap(queue);
    _tpWaitForSignalStart.store(waitForSignalStart);

    using namespace std::chrono_literals;

    m_threads.push_back(std::make_unique<std::thread>(worker, &m_taskQueue, "#w1#"));
    std::this_thread::sleep_for(200ms);
    m_threads.push_back(std::make_unique<std::thread>(worker, &m_taskQueue, "#w2#"));
    std::this_thread::sleep_for(200ms);
    m_threads.push_back(std::make_unique<std::thread>(worker, &m_taskQueue, "#w3#"));
    std::this_thread::sleep_for(200ms);
    m_threads.push_back(std::make_unique<std::thread>(worker, &m_taskQueue, "#w4#"));
}

void threadpool::ThreadPool::createThreads(std::uint32_t count)
{
    for (std::uint32_t i = 0; i < count; i++)
        m_threads.push_back(std::make_unique<std::thread>());
}
