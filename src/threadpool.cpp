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
std::mutex _tpMtCout;
#endif

using namespace threadpool;
using namespace std;

threadpool::PoolQueue::PoolQueue() {}

Worker::Worker(int id)
{
    status.store(NOAVAILABLE);
    this->id = id;
    tpLockPrint("create worker " << id);
}

int Worker::work(IThreadPool* pool, PoolQueue* queue)
{
    std::shared_ptr<threadpool::IRunnable> backElm = nullptr;
    do
    {
        {
            std::unique_lock lk{pool->m_mtQueue};
            status.store(WAITTING);
            tpLockPrint("worker " << this->id << " is waiting");
            pool->m_tpCV.wait(
                lk,
                [&]()
                {
                    return !pool->m_tpWaitForSignalStart.load(std::memory_order_relaxed) &&
                           (pool->m_tpTerminal.load(std::memory_order_relaxed) || !queue->empty());
                });

            if (pool->m_tpTerminal.load(std::memory_order_relaxed)) break;

            // get back element
            backElm = queue->front();
            queue->pop();

            lk.unlock();
        }

        if (backElm)
        {
            tpLockPrint("worker " << this->id << " is getting a task");
            status.store(BUSY);
            backElm->run();
        }
        backElm = nullptr;

    } while (true);

    status.store(END);
    tpLockPrint("worker " << this->id << " is exited");
    return 1;
}

int Worker::workFor(const std::chrono::nanoseconds& aliveTime, IThreadPool* pool, PoolQueue* queue)
{
    std::shared_ptr<threadpool::IRunnable> backElm = nullptr;
    do
    {
        {
            std::unique_lock lk{pool->m_mtQueue};
            status.store(WAITTING);
            tpLockPrint("s-worker " << this->id << " is waiting");
            if (!pool->m_tpCV.wait_for(
                    lk, aliveTime,
                    [&]()
                    {
                        return !pool->m_tpWaitForSignalStart.load(std::memory_order_relaxed) &&
                               (pool->m_tpTerminal.load(std::memory_order_relaxed) ||
                                !queue->empty());
                    }))
            {
                break;
            }

            if (pool->m_tpTerminal.load(std::memory_order_relaxed)) break;
            // get back element
            backElm = queue->front();
            queue->pop();

            lk.unlock();
        }

        if (backElm)
        {
            tpLockPrint("s-worker " << this->id << " is getting a task");
            status.store(BUSY);
            backElm->run();
        }
        backElm = nullptr;

    } while (true);

    status.store(END);
    tpLockPrint("s-worker " << this->id << " is exited");
    return 1;
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

void threadpool::ThreadPool::push(std::shared_ptr<IRunnable> runnable)
{
    cleanBackCompleteWorker();

    if (m_workers.size() < m_maxSize)
    {
        bool create = true;
        for (auto& w : m_workers)
        {
            if (w->status.load(std::memory_order_relaxed) == threadpool::WAITTING)
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

void threadpool::ThreadPool::terminate()
{
    m_tpTerminal.store(true);
    m_tpCV.notify_all();
}

void threadpool::ThreadPool::createWorker(std::uint32_t count)
{
    for (std::uint32_t i = 0; i < count; i++)
    {
        m_workers.push_back(std::make_unique<threadpool::Worker>(m_workers.size()));
        auto workerRaw = m_workers.back().get();
        m_threads.push_back(
            std::make_unique<std::thread>(&Worker::work, workerRaw, this, &m_taskQueue));
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
                                                          this, &m_taskQueue));
    }
}

bool ThreadPool::cleanBackCompleteWorker()
{
    if (m_workers.size() > 0)
    {
        std::size_t pos = m_workers.size() - 1;
        if (m_workers.at(pos)->status.load(memory_order::memory_order_relaxed) == WorkerStatus::END)
        {
            auto& thread = m_threads.at(pos);

            // validate again to make sure a worker is really ended;
            if (thread->joinable()) thread->join();

            // It is now safe to remove an employee who completed the mission
            m_workers.erase(m_workers.begin() + pos);
            m_threads.erase(m_threads.begin() + pos);
        }
        return true;
    }
    return false;
}

void ThreadPool::cleanCompleteWorker()
{
    auto workerSize = m_workers.size();

    for (std::size_t pos = workerSize - 1; pos >= 0; pos--)
    {
        if (m_workers.at(pos)->status.load(memory_order::memory_order_relaxed) == WorkerStatus::END)
        {
            auto& thread = m_threads.at(pos);
            if (thread->joinable())
            {
                thread->join(); // validate again to make sure a worker is really ended;
            }

            // It is now safe to remove an employee who completed the mission
            m_workers.erase(m_workers.begin() + pos);
            m_threads.erase(m_threads.begin() + pos);
        }
    }
}
