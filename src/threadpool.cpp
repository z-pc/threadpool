//////////////////////////////////////////////////////////////////////////
// File: threadpool.cpp
// Description: Implement for threadpool class
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

#include "threadpool.h"
#include <memory>
#include <thread>

#ifdef TP_CONSOLE
#include <iostream>
#include <sstream>
#endif

using namespace threadpool;
using namespace std;

threadpool::PoolQueue::PoolQueue() {}

Worker::Worker(std::uint32_t id)
{
    status.store(NOAVAILABLE);
    this->id = id;
    _tpLockPrint("create worker " << id);
}

int Worker::work(IThreadPool& pool, PoolQueue& queue)
{
    waitForStartSignal(pool);

    std::shared_ptr<threadpool::IRunnable> backElm = nullptr;
    do
    {
        {
            std::unique_lock lk{pool.m_mtQueue};
            status.store(WAITTING_TASK);
            _tpLockPrint("worker " << this->id << " is waiting for new task");

            pool.m_tpCV.wait(
                lk, [&]()
                { return (pool.m_tpTerminal.load() || !queue.empty()); });

            if (pool.m_tpTerminal.load()) break;
            backElm = queue.front();
            queue.pop();
            lk.unlock();
        }

        if (backElm)
        {
            _tpLockPrint("worker " << this->id << " is getting a task");
            status.store(BUSY);
            backElm->run();
        }
        backElm = nullptr;

    } while (true);

    _tpLockPrint("worker " << this->id << " is exited");
    status.store(END);
    return 1;
}

int Worker::workFor(const std::chrono::nanoseconds& aliveTime, IThreadPool& pool, PoolQueue& queue)
{
    waitForStartSignal(pool);

    std::shared_ptr<threadpool::IRunnable> backElm = nullptr;
    do
    {
        {
            std::unique_lock lk{pool.m_mtQueue};
            status.store(WAITTING_TASK);
            _tpLockPrint("s-worker " << this->id << " is waiting for new task");

            pool.m_tpCV.wait_for(lk, aliveTime,
                                 [&]() {
                                     return pool.m_tpTerminal.load() ||
                                            !queue.empty();
                                 });

            if (pool.m_tpTerminal.load() || queue.empty()) break;

            backElm = queue.front();
            queue.pop();
            lk.unlock();
        }

        if (backElm)
        {
            _tpLockPrint("s-worker " << this->id << " is getting a task");
            status.store(BUSY);
            backElm->run();
        }
        backElm = nullptr;

    } while (true);

    _tpLockPrint("s-worker " << this->id << " is exited");
    status.store(END);
    return 1;
}

void Worker::waitForStartSignal(IThreadPool& pool)
{
    std::unique_lock lk{pool.m_mtQueue};
    status.store(WAITTING_START);
    _tpLockPrint("worker " << this->id << " is waiting for start signal");
    pool.m_tpCV.wait(lk, [&]()
                     { return !pool.m_tpWaitForSignalStart.load(); });
    lk.unlock();
}

ThreadPool::ThreadPool(std::uint32_t poolSize /*= THREAD_POOl_DEFAULT_POOL_SIZE*/,
                       std::uint32_t poolMaxSize /*= std::thread::hardware_concurrency()*/,
                       const std::chrono::nanoseconds& aliveTime /*= 60s*/,
                       bool waitForSignalStart /*= false*/)
{
    m_coreSize = poolSize;
    m_maxSize = poolMaxSize;
    m_aliveTime = aliveTime;
    m_tpWaitForSignalStart.store(waitForSignalStart);
}

threadpool::ThreadPool::~ThreadPool() { terminate(); }

bool ThreadPool::isIdle()
{
    for (auto& w : m_workers)
        if (w->status.load() != threadpool::WAITTING_TASK) return false;

    return true;
}

bool threadpool::ThreadPool::push(std::shared_ptr<IRunnable> runnable)
{
    if (executable())
    {
        cleanCompleteWorker();

        if (m_workers.size() < m_maxSize)
        {
            bool create = true;
            for (auto& w : m_workers)
            {
                if (w->status.load() == threadpool::WAITTING_TASK)
                {
                    create = false;
                    break;
                }
            }

            if (create)
            {
                // Check if the number for main work is full, so we need to create seasonal workers.
                if (m_workers.size() >= m_coreSize)
                    createSeasonalWorker(1, m_aliveTime);
                else
                    createWorker(1);
            }
        }

        {
            std::lock_guard lk{m_mtQueue};
            m_taskQueue.push(runnable);
            m_tpCV.notify_one();
        }
        return true;
    }
    return false;
}

void threadpool::ThreadPool::start()
{
    m_tpWaitForSignalStart.store(false);
    m_tpCV.notify_all();
}

void threadpool::ThreadPool::wait()
{
    for (auto& th : m_threads)
        if (th->joinable()) th->join();
}

void ThreadPool::detach()
{
    for (auto& th : m_threads)
        th->detach();
}

void threadpool::ThreadPool::terminate()
{
    m_tpTerminal.store(true);
    m_tpCV.notify_all();
}

bool ThreadPool::executable() { return !m_tpTerminal.load(); }

void threadpool::ThreadPool::createWorker(std::uint32_t count)
{
    for (std::uint32_t i = 0; i < count; i++)
    {
        m_workers.push_back(std::make_unique<threadpool::Worker>(m_workers.size()));
        auto workerRaw = m_workers.back().get();
        m_threads.push_back(std::make_unique<std::thread>(&Worker::work, workerRaw, std::ref(*this),
                                                          std::ref(m_taskQueue)));
    }
}

void ThreadPool::createSeasonalWorker(std::uint32_t count,
                                      const std::chrono::nanoseconds& aliveTime)
{
    for (std::uint32_t i = 0; i < count; i++)
    {
        m_workers.push_back(std::make_unique<threadpool::Worker>(m_workers.size()));
        auto workerRaw = m_workers.back().get();
        m_threads.push_back(std::make_unique<std::thread>(&Worker::workFor, workerRaw, aliveTime,
                                                          std::ref(*this), std::ref(m_taskQueue)));
    }
}

void ThreadPool::cleanCompleteWorker()
{
    auto workerIt = m_workers.begin();
    auto threadIt = m_threads.begin();

    while (workerIt != m_workers.end())
    {
        auto& worker = *workerIt;
        auto& thread = *threadIt;

        if (worker->status.load() == WorkerStatus::END)
        {
            if (thread->joinable())
            {
                thread->join(); // validate again to make sure a worker is really ended;
            }

            // It is now safe to remove a worker who completed the mission
            workerIt = m_workers.erase(workerIt);
            threadIt = m_threads.erase(threadIt);
        }
        else
        {
            workerIt++;
            threadIt++;
        }
    }
}

ThreadPoolFixed::ThreadPoolFixed(std::uint32_t coreSize) : ThreadPool(coreSize, coreSize, 0s, true)
{
}

ThreadPoolFixed::~ThreadPoolFixed() {}

void ThreadPoolFixed::createWorker(std::uint32_t count) { createSeasonalWorker(count, 0s); }

bool ThreadPoolFixed::executable()
{
    if (ThreadPool::executable() == false)
        return false;
    else
    {
        cleanCompleteWorker();
        return m_workers.size() > 0;
    }
}
