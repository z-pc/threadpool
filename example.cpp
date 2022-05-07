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

        ThreadPool pool(1, 1, queue, 0, false);

        std::cout << "main thread sleep for 3s" << std::endl;
        std::this_thread::sleep_for(3s);
        pool.start();

        std::cout << "main thread sleep for 40s" << std::endl;
        std::this_thread::sleep_for(40s);
        pool.execute(RunnableExample("#t7#"));

        std::cout << "main thread sleep for 10s" << std::endl;
        std::this_thread::sleep_for(10s);

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
