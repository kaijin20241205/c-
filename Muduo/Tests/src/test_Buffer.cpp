#include <iostream>
using namespace std;

#include "../../include/gtest/gtest.h"
#include "../../Buffer.h"

#include <sys/resource.h>
#include <unistd.h>
#include <cstdlib>


class BufferTestFixture : public ::testing::Test
{
protected:
    void SetUp() override 
    {
        buffer_ = bufferInit();
    }
    void TearDown() override
    {
        bufferDestory(&buffer_);
    }
    static void SetUpTestSuite()
    {
        s_buffer_ = bufferInit();
    }
    static void TearDownTestSuite()
    {
        bufferDestory(&s_buffer_);
    }
    // 套件级别
    static Buffer* s_buffer_;
    // 测试用例级别
    Buffer* buffer_;
};

Buffer* BufferTestFixture::s_buffer_ = nullptr;

TEST(buffer_init, default_init_destory)
{
    Buffer* buffer = bufferInit();
    ASSERT_TRUE(buffer != NULL);
    ASSERT_TRUE(buffer->buffer_ != NULL);

    // EXPECT_NE(buffer->buffer_, NULL); 报错——>比较整数与指针？NULL-> #define NULL ((void *)0)
    EXPECT_EQ(buffer->headSpace_, 8);
    EXPECT_EQ(buffer->readIndex_, 8);
    EXPECT_EQ(buffer->writeIndex_, 8);
    EXPECT_EQ(bufferWriteableBytes(buffer), 4096);
    EXPECT_EQ(bufferReadableBytes(buffer), 0);
    EXPECT_EQ(buffer->capacity_, 4104);
    bufferDestory(&buffer);
    // EXPECT_TRUE(buffer->buffer_ == NULL);
    ASSERT_TRUE(buffer == NULL);
}

TEST(buffer_init, zero_size_init_destory)
{
    Buffer* buffer = bufferInit(0, 0);
    ASSERT_TRUE(buffer != NULL);
    // EXPECT_TRUE(buffer->buffer_ == NULL);
    EXPECT_TRUE(buffer->capacity_ == 0);
    EXPECT_TRUE(buffer->headSpace_ == 0);
    EXPECT_TRUE(buffer->readIndex_ == 0);
    EXPECT_TRUE(buffer->writeIndex_ == 0);
}

TEST(buffer_init, body_size_65536_init_destory)
{
    Buffer* buffer = bufferInit(20, 65536);
    EXPECT_EQ(buffer->capacity_, 65556);
    EXPECT_EQ(buffer->headSpace_, 20);
    EXPECT_EQ(buffer->readIndex_, 20);
    EXPECT_EQ(buffer->writeIndex_, 20);
    EXPECT_EQ(bufferWriteableBytes(buffer), 65536);
    EXPECT_EQ(bufferReadableBytes(buffer), 0);
    bufferDestory(&buffer);
    EXPECT_TRUE(buffer == NULL);
}

TEST(buffer_init, million_times_buffer_init_destory)
{
    // 跳过耗时太长
    GTEST_SKIP() << "million_times_buffer_init_destory skip...";
    int times = 1000000;
    Buffer** bufferList = (Buffer**)malloc(sizeof(Buffer*) * times);
    for (int i = 0; i < times; i++)
    {
        bufferList[i] = bufferInit();
    }
    srand(time(NULL));
    Buffer* buffer = bufferList[0];
    EXPECT_EQ(buffer->capacity_, 4104);
    EXPECT_TRUE(buffer->buffer_ != NULL);

    buffer = bufferList[times - 1];
    EXPECT_EQ(buffer->capacity_, 4104);
    EXPECT_TRUE(buffer->buffer_ != NULL);

    for (int i = 0; i < 10; i++)
    {
        int randomNum = rand() % times;
        buffer = bufferList[randomNum];
        EXPECT_EQ(buffer->capacity_, 4104);
        EXPECT_TRUE(buffer->buffer_ != NULL);
    }
    
    for (int i = 0; i < times; i++)
    {
        bufferDestory(&(bufferList[i]));
    }

    for (int i = 0; i < 10; i++)
    {
        int randomNum = rand() % times;
        buffer = bufferList[randomNum];
        EXPECT_TRUE(buffer == NULL);
    }
}

TEST(buffer_init, head_overflow)
{
    // 跳过，会影响到其他的测试用例
    GTEST_SKIP() << "head_overflow skip...";
    // 限制进程最大堆内存为10MB
    struct rlimit rl;
    rl.rlim_cur = rl.rlim_max = 10 * 1024 * 1024;  // 10MB
    setrlimit(RLIMIT_DATA, &rl);
    Buffer* buffer = bufferInit(20, 10 * 1024 * 1024);
    EXPECT_TRUE(buffer == NULL);
}

TEST_F(BufferTestFixture, buffer_expand)
{
    bufferExpandByest(4096, buffer_);
    ASSERT_TRUE(buffer_->buffer_ != NULL);
    ASSERT_TRUE(buffer_->capacity_ == 4096 + 8 + 4096);
    ASSERT_TRUE(bufferWriteableBytes(buffer_) == 4096 + 4096);
    EXPECT_TRUE(buffer_->headSpace_ == 8);
    EXPECT_TRUE(buffer_->readIndex_ == 8);
    EXPECT_TRUE(buffer_->writeIndex_ == 8);
    EXPECT_TRUE(bufferReadableBytes(buffer_) == 0);
}

TEST_F(BufferTestFixture, buffer_expand_zero)
{
    bufferExpandByest(0, buffer_);
    ASSERT_TRUE(buffer_->buffer_ != NULL);
    ASSERT_TRUE(buffer_->capacity_ == 4096 + 8 + 0);
    EXPECT_TRUE(bufferWriteableBytes(buffer_) == 4096 + 0);
    EXPECT_TRUE(buffer_->headSpace_ == 8);
    EXPECT_TRUE(buffer_->readIndex_ == 8);
    EXPECT_TRUE(buffer_->writeIndex_ == 8);
    EXPECT_TRUE(bufferReadableBytes(buffer_) == 0);
}

TEST_F(BufferTestFixture, buffer_expand_65536)
{
    bufferExpandByest(65536, buffer_);
    ASSERT_TRUE(buffer_->buffer_ != NULL);
    ASSERT_TRUE(buffer_->capacity_ == 4096 + 8 + 65536);
    EXPECT_TRUE(bufferWriteableBytes(buffer_) == 4096 + 65536);
    EXPECT_TRUE(buffer_->headSpace_ == 8);
    EXPECT_TRUE(buffer_->readIndex_ == 8);
    EXPECT_TRUE(buffer_->writeIndex_ == 8);
    EXPECT_TRUE(bufferReadableBytes(buffer_) == 0);
}

TEST_F(BufferTestFixture, buffer_expand_million)
{
    GTEST_SKIP() << "buffer_expand_million skip...";
    // 跳过耗时太长
    bufferExpandByest(65536, buffer_);
    int times = 1000;
    for (int i = 0; i < times; i++)
    {
        bufferExpandByest(1024, buffer_);
    }
    EXPECT_TRUE(buffer_->capacity_ == 8 + 4096 + 65536 + 1024 * times);
    EXPECT_TRUE(bufferWriteableBytes(buffer_) == 4096 + 65536 + 1024 * times);
}

TEST_F(BufferTestFixture, buffer_append)
{
    const char* msg = "hello world";
    ssize_t msgLen = strlen(msg);
    bufferAppend("hello world", msgLen, s_buffer_);
    EXPECT_TRUE(bufferReadableBytes(s_buffer_) == msgLen);
    EXPECT_TRUE(bufferWriteableBytes(s_buffer_) == 4096 - msgLen);
    
    int times = 399;
    for (int i = 0; i < times; i++)
    {
        bufferAppend(msg, msgLen, s_buffer_);
    }
    EXPECT_TRUE(bufferReadableBytes(s_buffer_) == msgLen * (times + 1));
    EXPECT_TRUE(s_buffer_->readIndex_ == s_buffer_->headSpace_);
    EXPECT_TRUE(s_buffer_->writeIndex_ == s_buffer_->headSpace_ + msgLen * (times + 1));
    EXPECT_TRUE(bufferWriteableBytes(s_buffer_) == 0);
}

TEST_F(BufferTestFixture, buffer_retrieve_all)
{
    ssize_t strLen = bufferReadableBytes(s_buffer_);
    char* msg = bufferRetrieveAllAsString(s_buffer_);
    ASSERT_TRUE(msg != NULL);
    ASSERT_TRUE(strLen == strlen(msg));
    EXPECT_TRUE(bufferReadableBytes(s_buffer_) == 0);
    free(msg);
}

TEST(buffer_retrieve, buffer_retrieve_zero)
{
    Buffer* buffer = bufferInit();
    const char* msg = "hello world";
    ssize_t msgLen = strlen(msg);
    bufferAppend(msg, msgLen, buffer);
    ssize_t enableWrite = bufferWriteableBytes(buffer);
    char* retrieveZeroMsg = bufferRetrieveAsString(0, buffer);
    ASSERT_TRUE(bufferReadableBytes(buffer) == msgLen);
    ASSERT_TRUE(bufferWriteableBytes(buffer) == enableWrite);
    if (retrieveZeroMsg != NULL)
    {
        free(retrieveZeroMsg);
    }
    bufferDestory(&buffer);
}

TEST(buffer_retrieve, buffer_retrieves)
{
    Buffer* buffer = bufferInit();
    const char* msg = "hello world";
    ssize_t msgLen = strlen(msg);
    bufferAppend(msg, msgLen, buffer);
    char* retrieveAllMsg = bufferRetrieveAsString(msgLen, buffer);
    ASSERT_TRUE(msgLen == strlen(retrieveAllMsg));
    ASSERT_TRUE(strcmp(msg, retrieveAllMsg) == 0);
    ASSERT_TRUE(bufferWriteableBytes(buffer) == 4096);
    ASSERT_TRUE(bufferReadableBytes(buffer) == 0);
    bufferAppend(msg, msgLen, buffer);
    for (int i = 0; i < msgLen; i++)
    {
        char* p = bufferRetrieveAsString(1, buffer);
        ASSERT_TRUE(strncmp(p, &(msg[i]), 1) == 0);
        ASSERT_TRUE(*p == msg[i]);
        free(p);
    }
    char* p = bufferRetrieveAsString(10, buffer);
    ASSERT_TRUE(strcmp(p, "\0") == 0);
    ASSERT_TRUE(bufferReadableBytes(buffer) == 0);
    ASSERT_TRUE(bufferWriteableBytes(buffer) == 4096);
    while (bufferReadableBytes(buffer) != 100)
    {
        bufferAppend("hello", 5, buffer);
    }
    ASSERT_TRUE(bufferReadableBytes(buffer) == 100);
    ASSERT_TRUE(bufferWriteableBytes(buffer) == 3996);
    bufferDestory(&buffer);
}

TEST(buffer, buffer_ensure)
{
    Buffer* buffer = bufferInit(8, 16);
    const char* msg = "hello world";
    ssize_t msgLen = strlen(msg);
    bufferAppend(msg, msgLen, buffer);
    char* retrieveMsg = bufferRetrieveAsString(6, buffer);
    free(retrieveMsg);
    ASSERT_TRUE(bufferWriteableBytes(buffer) == 5);
    ASSERT_TRUE(bufferReadableBytes(buffer) == 5);
    ensureWriteBytes(msgLen, buffer);
    ASSERT_TRUE(bufferWriteableBytes(buffer) == 11);
    bufferAppend(msg, msgLen, buffer);
    retrieveMsg = bufferRetrieveAllAsString(buffer);
    ASSERT_TRUE(strcmp(retrieveMsg, "worldhelloworld"));
    free(retrieveMsg);
    ASSERT_TRUE(bufferWriteableBytes(buffer) == 16);
    ASSERT_TRUE(bufferReadableBytes(buffer) == 0);
    bufferAppend(msg, msgLen, buffer);
    ASSERT_TRUE(bufferWriteableBytes(buffer) == 5);
    ASSERT_TRUE(bufferReadableBytes(buffer) == msgLen);
    ensureWriteBytes(11, buffer);
    ASSERT_TRUE(bufferReadableBytes(buffer) == msgLen);
    ASSERT_TRUE(bufferWriteableBytes(buffer) == 11);
    bufferAppend(msg, msgLen, buffer);
    retrieveMsg = bufferRetrieveAllAsString(buffer);
    ASSERT_TRUE(strcmp(retrieveMsg, "hello worldhello world") == 0);
    free(retrieveMsg);
    ASSERT_TRUE(bufferWriteableBytes(buffer) == 22);
    ASSERT_TRUE(bufferReadableBytes(buffer) == 0);
    bufferDestory(&buffer);
}


int main(int argc, char** argv)
{
    // 初始化googletest
    ::testing::InitGoogleTest(&argc, argv);
    // 运行所有测试用例
    return RUN_ALL_TESTS();
}
