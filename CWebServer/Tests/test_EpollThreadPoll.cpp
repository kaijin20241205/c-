#include "./include/gtest/gtest.h"
#include "./../include/EpollThreadPoll.h"

#include <iostream>
using namespace std;


TEST(epoll_thread_poll, thread_num_zero)
{
    EpollThreadPoll* p = epollThreadPollInit(0);
    ASSERT_TRUE(p->mainThreadEpfd_ >= 0);
    ASSERT_TRUE(p->destoryFlag == false);
    ASSERT_TRUE(p->evnetFd_ == 0);
    ASSERT_TRUE(p->subThreadEpfdsIndex_ == 0);
    ASSERT_TRUE(p->subThtreadEpfds_ == NULL);
    ASSERT_TRUE(p->threadNum_ == 0);
    ASSERT_TRUE(p->threads_ == NULL);
    ASSERT_TRUE(getNextSubThtreadEpfd(p) >= 0);     // 0、1、2系统预留
    epollThreadPollStart(p);
    epollThreadPollDestory(&p);
    ASSERT_TRUE(p == NULL);
}

TEST(epoll_thread_poll, thread_num_normal)
{
    GTEST_SKIP() << "need wait epoll_wait 10second....";
    int testThreadNum = 4;
    EpollThreadPoll* p = epollThreadPollInit(testThreadNum);
    ASSERT_TRUE(p->mainThreadEpfd_ >= 0);
    ASSERT_TRUE(p->destoryFlag == false);
    ASSERT_TRUE(p->evnetFd_ > 2);      
    ASSERT_TRUE(p->subThreadEpfdsIndex_ == 0);      
    ASSERT_TRUE(p->subThtreadEpfds_ != NULL);
    ASSERT_TRUE(p->threadNum_ == testThreadNum);
    ASSERT_TRUE(p->threads_ != NULL);
    epollThreadPollStart(p);
    ASSERT_TRUE(p->evnetFd_ == -1);
    ASSERT_TRUE(getNextSubThtreadEpfd(p) == 5);
    ASSERT_TRUE(getNextSubThtreadEpfd(p) == 6);
    ASSERT_TRUE(getNextSubThtreadEpfd(p) == 7);
    ASSERT_TRUE(getNextSubThtreadEpfd(p) == 8);
    ASSERT_TRUE(getNextSubThtreadEpfd(p) == 5);
    epollThreadPollDestory(&p);
    ASSERT_TRUE(p == NULL);
}

TEST(epoll_thread_poll, thread_num_50)
{
    GTEST_SKIP() << "need wait epoll_wait 10second....";
    int testThreadNum = 50;
    EpollThreadPoll* p = epollThreadPollInit(testThreadNum);
    epollThreadPollStart(p);
    epollThreadPollDestory(&p);
    ASSERT_TRUE(p == NULL);
}
