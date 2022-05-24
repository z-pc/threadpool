#include "src/threadpool.h"

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
            _tpLockPrint(m_name << " running " << i);
            std::this_thread::sleep_for(1s);
        }

        return true;
    }

    std::string m_name;
};
