#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

int main(void)
{
    try
    {
    }
    catch (const exception& e)
    {
        cout << e.what() << endl;
    }
    system("pause");
    return 0;
}
