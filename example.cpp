#include "RunnableExample.h"
#include "src/threadpool.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;
using namespace threadpool;

#define tp_sleep_for(time)                                                                         \
    _tpLockPrint("main thread sleep for" << ((std::chrono::seconds)time).count() << "s");          \
    std::this_thread::sleep_for(time);

#define tp_push_taks(name)                                                                         \
    _tpLockPrint("main thread push" << name);                                                      \
    pool.push(std::make_shared<RunnableExample>(name));

void testThreadPool()
{
    ThreadPool pool = ThreadPool(1, 3, 20s);

    tp_sleep_for(1s);
    tp_push_taks("#t1");
    tp_push_taks("#t2");
    tp_push_taks("#t3");
    tp_sleep_for(10s);
    tp_push_taks("#t4");

    _tpLockPrint("terminate");
    pool.terminate();

    tp_push_taks("#t5");
    tp_push_taks("#t6");

    cout << pool.isIdle() << endl;

    pool.wait();
}

void testThreadPoolFixed()
{
    ThreadPoolFixed pool = ThreadPoolFixed(2);

    pool.emplace<RunnableExample>("#test emplace#");

    tp_sleep_for(1s);
    tp_push_taks("#t1");
    tp_push_taks("#t2");
    tp_push_taks("#t3");
    tp_push_taks("#t4");
    tp_push_taks("#t5");
    tp_sleep_for(5s);
    pool.start();

    tp_sleep_for(5s);
    _tpLockPrint("terminate");
    pool.terminate();

    pool.wait();
    cout << "Wait done" << endl;
}

void testTerminate()
{
    ThreadPool pool = ThreadPool(1, 3, 20s);

    tp_push_taks("#t1");
    tp_sleep_for(2s);

    tp_push_taks("#t2");

    tp_sleep_for(2s);
    _tpLockPrint("terminate");
    pool.terminate();

    tp_push_taks("#t3");
    tp_push_taks("#t4");

    pool.wait();
    cout << "wait done" << endl;
}

int main(void)
{

    testTerminate();

    cout << "Press any key to continue...";
    getchar();
    return 0;
}
