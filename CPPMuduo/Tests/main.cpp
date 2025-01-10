#include <iostream>

#include "./include/gtest/gtest.h"
#include "./../include/Logger.hpp"
#include "./../include/Timestamp.hpp"

using namespace std;


int main(int argc, char** argv)
{
    // 初始化googletest
    ::testing::InitGoogleTest(&argc, argv);
        // cout << time(nullptr) << endl;

    LOG_INFO("%s%d", "hello", 1);
    LOG_ERROR("%s%d", "hello", 2);
    // LOG_FATAL("%s%d", "hello", 2);
    LOG_DEBUG("%s%d", "hello", 4);

    LOG_DEBUG("%s%d", "hello", 5);
    LOG_DEBUG("%s%d", "hello", 6);

    cout << Timestamp::now().toString() << endl;
    cout << Timestamp::now().toString() << endl;
    RUN_ALL_TESTS();
    //test
    return 0;
}