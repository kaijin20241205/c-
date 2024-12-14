#include <iostream>
using namespace std;

void test(const char* str, ...)
{
    char buf[1024] = {0};
    snprintf(buf, 1024, str, 1000, "hello");
    cout << buf << endl;
}


int main() 
{   
    test("hello, %d, %s", 100, "hello");
    return 0;
}