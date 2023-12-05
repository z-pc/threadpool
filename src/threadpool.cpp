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

using namespace athread;
using namespace std;

athread::PoolQueue::PoolQueue() {}

Worker::Worker(std::uint32_t id)
{
    status.store(NOAVAILABLE);
    this->id = id;
    _tpLockPrint("create worker " << id);
}

int Worker::work(ThreadPool& pool, PoolQueue& queue)
{
    waitForStartSignal(pool);

    athread::Runnable* frontElm = nullptr;
    do
    {
        {
            std::unique_lock<std::mutex> lk{pool.m_queueLocker};
            status.store(WAITTING_TASK);
            _tpLockPrint("worker " << this->id << " is waiting for new task");

            pool.m_cv.wait(lk, [&]()
                           { return (pool.m_terminalSignal.load() || !queue.empty()); });

            if (pool.m_terminalSignal.load())
            {
                lk.unlock();
                break;
            }
            frontElm = queue.front();
            queue.pop();
            lk.unlock();
        }

        if (frontElm)
        {
            _tpLockPrint("worker " << this->id << " is getting a task");
            status.store(BUSY);
            frontElm->run();
            delete frontElm;
        }

        frontElm = nullptr;

    } while (true);

    _tpLockPrint("worker " << this->id << " is exited");
    status.store(END);
    return 1;
}

int Worker::workFor(const std::chrono::nanoseconds& aliveTime, ThreadPool& pool, PoolQueue& queue)
{
    waitForStartSignal(pool);

    athread::Runnable* frontElm = nullptr;
    do
    {
        {
            std::unique_lock<std::mutex> lk{pool.m_queueLocker};
            status.store(WAITTING_TASK);
            _tpLockPrint("s-worker " << this->id << " is waiting for new task");

            pool.m_cv.wait_for(lk, aliveTime,
                               [&]()
                               { return pool.m_terminalSignal.load() || !queue.empty(); });

            if (pool.m_terminalSignal.load() || queue.empty()) break;

            frontElm = queue.front();
            queue.pop();
            lk.unlock();
        }

        if (frontElm)
        {
            _tpLockPrint("s-worker " << this->id << " is getting a task");
            status.store(BUSY);
            frontElm->run();
            delete frontElm;
        }
        frontElm = nullptr;

    } while (true);

    _tpLockPrint("s-worker " << this->id << " is exited");
    status.store(END);
    return 1;
}

void Worker::waitForStartSignal(ThreadPool& pool)
{
    std::unique_lock<std::mutex> lk{pool.m_queueLocker};
    status.store(WAITTING_START);
    _tpLockPrint("worker " << this->id << " is waiting for start signal");
    pool.m_cv.wait(lk, [&]()
                   { return !pool.m_waitForSignalStart.load(); });
    lk.unlock();
}

ThreadPool::ThreadPool(std::uint32_t coreSize /*= THREAD_POOl_DEFAULT_POOL_SIZE*/,
                       int maxSize /*= std::thread::hardware_concurrency()*/,
                       const std::chrono::nanoseconds& aliveTime /*= 60s*/,
                       bool waitForSignalStart /*= false*/)
{
    m_coreSize = coreSize;
    m_maxSize = maxSize;
    m_aliveTime = aliveTime;
    m_waitForSignalStart.store(waitForSignalStart);
}

athread::ThreadPool::~ThreadPool() { terminate(); }

bool ThreadPool::isIdle()
{
    for (auto& w : m_workers)
        if (w->status.load() != athread::WAITTING_TASK) return false;

    return true;
}

bool athread::ThreadPool::push(Runnable* runnable)
{
    if (executable())
    {
        cleanCompleteWorkers();

        if (m_workers.size() < m_maxSize || m_maxSize < 0)
        {
            bool create = true;
            for (auto& w : m_workers)
            {
                if (w->status.load() == athread::WAITTING_TASK)
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
            std::lock_guard<std::mutex> lk{m_queueLocker};
            m_taskQueue.push(runnable);
            m_cv.notify_one();
        }
        return true;
    }
    return false;
}

void athread::ThreadPool::start()
{
    m_waitForSignalStart.store(false);
    m_terminalSignal.store(false);
    m_cv.notify_all();
}

void athread::ThreadPool::wait()
{
    cleanCompleteWorkers();
    for (auto& th : m_threads)
        if (th && th->joinable()) th->join();
}

void ThreadPool::detach()
{
    cleanCompleteWorkers();
    for (auto& th : m_threads)
        th->detach();
}

void athread::ThreadPool::terminate(bool alsoWait)
{
    m_terminalSignal.store(true);
    m_cv.notify_all();
    if (alsoWait) wait();
}

bool ThreadPool::executable() { return !m_terminalSignal.load(); }

void athread::ThreadPool::createWorker(std::uint32_t count)
{
    for (std::uint32_t i = 0; i < count; i++)
    {
        m_workers.push_back(std::unique_ptr<athread::Worker>(new Worker(m_workers.size())));
        auto workerRaw = m_workers.back().get();
        m_threads.push_back(std::unique_ptr<std::thread>(
            new std::thread(&Worker::work, workerRaw, std::ref(*this), std::ref(m_taskQueue))));
    }
}

void ThreadPool::createSeasonalWorker(std::uint32_t count,
                                      const std::chrono::nanoseconds& aliveTime)
{
    for (std::uint32_t i = 0; i < count; i++)
    {
        m_workers.push_back(std::unique_ptr<athread::Worker>(new Worker(m_workers.size())));
        auto workerRaw = m_workers.back().get();
        m_threads.push_back(std::unique_ptr<std::thread>(new std::thread(
            &Worker::workFor, workerRaw, aliveTime, std::ref(*this), std::ref(m_taskQueue))));
    }
}

void athread::ThreadPool::cleanCompleteWorkers()
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
    if (m_terminalSignal.load() == true) return false;
    if (m_waitForSignalStart.load() == true) return true;
    // cleanCompleteWorker();
    return m_workers.size() > 0;
}
