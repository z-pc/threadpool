#include "src/threadpool.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;
using namespace threadpool;

int main(void)
{
    try
    {
        PoolQueue queue;
        queue.push(std::make_shared<RunnableExample>("#t1#"));
        queue.push(std::make_shared<RunnableExample>("#t2#"));
        queue.push(std::make_shared<RunnableExample>("#t3#"));
        queue.push(std::make_shared<RunnableExample>("#t4#"));
        queue.push(std::make_shared<RunnableExample>("#t5#"));
        queue.push(std::make_shared<RunnableExample>("#t6#"));

        ThreadPool pool = ThreadPool(2);
        ThreadPool pool2 = ThreadPool(queue, 2);
        pool.execute(std::make_shared<RunnableExample>("#t7#"));
        pool.execute(std::make_shared<RunnableExample>("#t8#"));
        pool.execute(std::make_shared<RunnableExample>("#t9#"));

        ThreadPoolDynamic pool1 = ThreadPoolDynamic();

        std::cout << "main thread sleep for 10s" << std::endl;
        std::this_thread::sleep_for(20s);

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
