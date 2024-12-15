#include <iostream>
using namespace std;

void test(char** pp)
{
    cout << "*pp:" << *pp << endl;
    delete[] *pp;
    *pp = new char[3]{2, 2, 2};
    for (int i = 0; i < 3; i++)
    {
        cout << (*pp[i]) << endl;
    }
    
}


int main() 
{   
    char* p = new char[3]{'1', '2', '3'};
    for (int i = 0; i < 3; i++)
    {
        cout << p[i] << endl;
    }
    char** pp = &p;
    for (int i = 0; i < 3; i++)
    {
        cout << *(pp+i) << endl;
    }
    cout << *pp << endl;
    return 0;
}