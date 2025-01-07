#include <iostream>

#include "./include/gtest/gtest.h"

using namespace std;


TEST(test, google_test)
{
    ASSERT_TRUE(true);
}

int main(int argc, char** argv)
{
    // 初始化googletest
    ::testing::InitGoogleTest(&argc, argv);

    RUN_ALL_TESTS();

    return 0;
}