#include "src/threadpool.h"

class RunnableExample : public threadpool::IRunnable
{
public:
    RunnableExample(std::string name)
    {
        srand(time(NULL));
        m_name = name;
        loop = rand() % 10 + 2;
        _tpLockPrint(m_name << " loop " << loop);
    };
    ~RunnableExample(){};

    virtual bool run() override
    {
        using namespace std::chrono_literals;        

        for (int i = 0; i < loop; i++)
        {
            _tpLockPrint(m_name << " running " << i);
            std::this_thread::sleep_for(1s);
        }

        return true;
    }

    std::string m_name;
    int loop;
};
