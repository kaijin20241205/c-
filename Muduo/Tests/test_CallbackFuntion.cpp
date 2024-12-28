#include "../../include/gtest/gtest.h"
#include "CallbackFunction.h"
#include "EpollEventCallback.h"
#include "EpollThreadPoll.h"

#include <sys/socket.h>      
#include <arpa/inet.h>            
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/resource.h>



class CallbackFuntionFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        int lfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        int optval = 1;
        setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)); 
        sockaddr_in localAddr{};
        localAddr.sin_port = htons(8888);
        inet_pton(AF_INET, "127.0.0.1", &localAddr.sin_addr.s_addr);
        localAddr.sin_family = AF_INET;
        socklen_t len = sizeof(localAddr);
        bind(lfd, (sockaddr*)&localAddr, len);
        listen(lfd, 128) == -1;

        epoll_event epv;
        memset(&epv, 0x00, sizeof(epoll_event));

        epollThreadPoll_ = epollThreadPollInit(0);

        lfdEpvCallback_ = epvCallbackInit(lfd, 
                                                EPOLLIN, 
                                                acceptor, 
                                                epollThreadPoll_->mainThreadEpfd_, 
                                                epollThreadPoll_);
        epv.events = EPOLLIN;
        epv.data.ptr = (void*)&lfdEpvCallback_;
        epollAdd((void*)lfdEpvCallback_);

        eventFd_ = eventfd(0, 0);

    }
    void TearDown() override
    {
        close(lfdEpvCallback_->epfd_);
        epvCallbackDestory(&lfdEpvCallback_);
        epollThreadPollDestory(&epollThreadPoll_);
    }
    EpvCallback* lfdEpvCallback_;
    EpollThreadPoll* epollThreadPoll_;
    int eventFd_;
};

struct threadWorkData
{
    int threaedNum;
    int eventFd;
};

void* threadWorkConnect(void* data)
{
    threadWorkData* workData = (threadWorkData*)data;
    
    int fds[workData->threaedNum];
    memset(fds, 0x00, sizeof(int) * workData->threaedNum);
    for (int i = 0; i < workData->threaedNum; i++)
    {
        int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        fds[i] = fd;
    }
    sockaddr_in addr;
    memset(&addr, 0x00, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    uint64_t value;
    read(workData->eventFd, &value, sizeof(value));
    for (int i = 0; i < workData->threaedNum; i++)
    {
        connect(fds[i], (sockaddr*)&addr, sizeof(sockaddr_in));
    }
    for (int i = 0; i < workData->threaedNum; i++)
    {
        close(fds[i]);
    }
    return NULL;
    
}


TEST_F(CallbackFuntionFixture, test_acceptor_one)
{
    epoll_event epv;
    memset(&epv, 0x00, sizeof(epoll_event));
    pthread_t pthread;
    threadWorkData data;
    data.eventFd = eventFd_;
    data.threaedNum = 1;
    pthread_create(&pthread, NULL, threadWorkConnect, (void*)&data);
    uint64_t message = 1;
    write(eventFd_, &message, sizeof(uint64_t));
    int n = epoll_wait(lfdEpvCallback_->epfd_, &epv, 1, 10000);
    ASSERT_TRUE(n == 1);
    EpvCallback* rEpvCallback = (EpvCallback*)epv.data.ptr;
    lfdEpvCallback_->callback_(lfdEpvCallback_->fd_, rEpvCallback->events_, epv.data.ptr);
}






