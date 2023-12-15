#include "threadpool.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int main(void)
{
    ThreadPool pool(1, 2);
    pool.push(new RunnableSample("task-1"));
    tp_sleep_for(1s);
    pool.push(new RunnableSample("task-2"));

    std::this_thread::sleep_for(1s);
    pool.terminate();

    pool.wait();
    return 0;
}
