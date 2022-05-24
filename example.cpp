#include "RunnableExample.h"
#include "src/threadpool.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;
using namespace threadpool;

#define tp_sleep_for(time)                                                                         \
    _tpLockPrint("main thread sleep for" << ((std::chrono::seconds)time).count() << "s");           \
    std::this_thread::sleep_for(time);

#define tp_push_taks(name)                                                                         \
    _tpLockPrint("main thread push" << name);                                                       \
    pool.push(std::make_shared<RunnableExample>(name));

// int main(void)
//{
//    srand(time(NULL));
//
//    try
//    {
//        ThreadPool pool = ThreadPool(3, 6, 6s);
//
//        pool.emplace<RunnableExample>("#test emplace#");
//
//        tp_sleep_for(1s);
//        tp_push_taks("#t1");
//        tp_push_taks("#t2");
//        tp_push_taks("#t3");
//        tp_push_taks("#t4");
//        tp_sleep_for(3s);
//        tp_push_taks("#t5");
//        tp_sleep_for(2s);
//        tp_push_taks("#t6");
//        tp_push_taks("#t7");
//        tp_push_taks("#t8");
//
//        tp_sleep_for(60s);
//
//        // someone somewhere in this galaxy say: "terminate"
//        pool.terminate();
//
//        // okey, but before I hug my mother last
//        pool.wait();
//    }
//    catch (const exception& e)
//    {
//        cout << e.what() << endl;
//    }
//    system("pause");
//    return 0;
//}

int main(void)
{
    srand(time(NULL));

    try
    {
        ThreadPoolFixed pool = ThreadPoolFixed(3);

        pool.emplace<RunnableExample>("#test emplace#");

        tp_sleep_for(1s);
        tp_push_taks("#t1");
        tp_push_taks("#t2");
        tp_push_taks("#t3");
        tp_push_taks("#t4");

        tp_sleep_for(60s);

        cout << pool.isIdle() << endl;

        // someone somewhere in this galaxy say: "terminate"
        pool.terminate();
        // okey, but before I hug my mother last
        pool.wait();
    }
    catch (const exception& e)
    {
        cout << e.what() << endl;
    }
    system("pause");
    return 0;
}
