#include "src/threadpool.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;
using namespace threadpool;

class RunnableExample : public threadpool::IRunnable
{
public:
    RunnableExample(std::string name) { m_name = name; };
    ~RunnableExample(){};

    virtual bool run() override
    {
        using namespace std::chrono_literals;
        int loop = rand() % 10 + 1;

        for (int i = 0; i < loop; i++)
        {
            {
                std::cout << m_name << " running " << i << std::endl;
            }
        }

        return true;
    }

    std::string m_name;
};


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
        pool.execute(std::make_shared<RunnableExample>("#t7#"));
        pool.execute(std::make_shared<RunnableExample>("#t8#"));
        pool.execute(std::make_shared<RunnableExample>("#t9#"));

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
