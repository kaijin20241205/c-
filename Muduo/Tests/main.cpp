#include "../../include/gtest/gtest.h"

#include <iostream>
using namespace std;

class CheckLeaks : public ::testing::Environment
{
public:
    void SetUp() override 
    {
    }
    void TearDown() override
    {
    }
    int num_ = 0;

};






int main(int argc, char** argv)
{
    // 初始化googletest
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CheckLeaks());
    // 运行所有测试用例
    RUN_ALL_TESTS();
    // check_leaks();
    return 0;
}