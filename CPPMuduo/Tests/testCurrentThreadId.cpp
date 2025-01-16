#include "./include/gtest/gtest.h"
#include "./../include/CurrentThreadId.hpp"

#include <iostream>
using namespace std;


void* test(void* num)
{
    int* n = (int*)num;
    int thread_id = CurrentThread::tid();
    cout << "n:" << *n << "thread_id:" << thread_id << endl;
    sleep(1);
    thread_id = CurrentThread::tid();
    cout << "n:" << *n << "thread_id:" << thread_id << endl;
    return NULL;
}


TEST(ThreadId, simple_test)
{
    int thread_id = CurrentThread::tid();
    cout << "n:" << 0 << "thread_id:" << thread_id << endl;

    pthread_t threas[2];
    int num1 = 1, num2 = 2;
    pthread_create(&threas[0], NULL, test, (void*)&num1);
    pthread_create(&threas[1], NULL, test, (void*)&num2);
    pthread_join(threas[0], NULL);
    pthread_join(threas[1], NULL);
    thread_id = CurrentThread::tid();
    cout << "n:" << 0 << "thread_id:" << thread_id << endl;
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}