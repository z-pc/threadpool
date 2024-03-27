#include "threadpool.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

using namespace std;
using namespace athread;

int main(void)
{
    ThreadPoolFixed pool(2);
    int a=4;
    pool.push([=]( )
                 { cout << "lambda function: " << a << endl; });
    pool.start();
    pool.wait();

    return 0;
}
