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
    pool.push([]()
              {
                  _tpLockPrint("auto_shutdown: run taks 1");
                  this_thread::sleep_for(1s);
                  _tpLockPrint("auto_shutdown: end taks 1");
              });
    pool.push([]()
              {
                  _tpLockPrint("auto_shutdown: run taks 2");
                  this_thread::sleep_for(1s);
                  _tpLockPrint("auto_shutdown: end taks 2");
              });
    pool.start();
    pool.wait();
    return 0;
}
