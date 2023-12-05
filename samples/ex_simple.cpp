#include "threadpool.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crtdbg.h>
#include <stdlib.h>

using namespace std;
using namespace athread;

class RunnableSample : public athread::Runnable
{
public:
    RunnableSample(std::string name)
    {
        srand(time(NULL));
        m_name = name;
        loop = rand() % 10 + 2;
        _tpLockPrint(m_name << " loop " << loop);
    };
    ~RunnableSample(){};

    virtual void run() override
    {
        using namespace std::chrono_literals;

        for (int i = 0; i < loop; i++)
        {
            _tpLockPrint(m_name << " running " << i);
            std::this_thread::sleep_for(1s);
        }
    }

    std::string m_name;
    int loop;
};

#define tp_sleep_for(time)                                                                \
    _tpLockPrint("main thread sleep for" << ((std::chrono::seconds)time).count() << "s"); \
    std::this_thread::sleep_for(time);

#define tp_push_taks(name)                    \
    _tpLockPrint("main thread push" << name); \
    pool.push(new RunnableSample(name));

void testThreadPool()
{
    ThreadPool pool(1, 3);

    tp_sleep_for(1s);
    tp_push_taks("#t1");
    tp_push_taks("#t2");
    tp_push_taks("#t3");
    tp_sleep_for(10s);
    tp_push_taks("#t4");

    _tpLockPrint("terminate");
    pool.terminate(false);

    tp_push_taks("#t5");
    tp_push_taks("#t6");

    cout << pool.isIdle() << endl;

    pool.wait();
}

void testThreadPoolFixed()
{
    ThreadPoolFixed pool(2);

    tp_push_taks("#t1");
    tp_sleep_for(1s);
    tp_push_taks("#t2");
    tp_sleep_for(1s);
    tp_push_taks("#t3");
    tp_sleep_for(1s);
    tp_push_taks("#t4");
    tp_sleep_for(1s);
    tp_push_taks("#t5");
    tp_sleep_for(1s);
    pool.start();

    tp_sleep_for(5s);
    _tpLockPrint("terminate");
    pool.terminate();
    cout << "Wait done" << endl;
}

void testTerminate()
{
    ThreadPool pool(1, 3, 20s);

    tp_push_taks("#t1");
    tp_sleep_for(1s);
    tp_push_taks("#t2");

    tp_sleep_for(2s);
    _tpLockPrint("terminate");

    tp_push_taks("#t3");
    tp_push_taks("#t4");

    pool.wait();
    cout << "wait done" << endl;
}

int main(void)
{
    {
        //auto tst = new int(6);
        ThreadPool pool(1, 2);
        pool.push(new RunnableSample("task-1"));
        tp_sleep_for(1s);
        pool.push(new RunnableSample("task-2"));

        std::this_thread::sleep_for(1s);
        pool.terminate();
    }
    //cout << "Press any key to continue...";
    //getchar();
    _CrtDumpMemoryLeaks();
    return 0;
}
