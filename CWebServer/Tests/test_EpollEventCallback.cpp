#include "./include/gtest/gtest.h"
#include "./../include/EpollEventCallback.h"

#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>


void testCallback1(int fd, int revents, void* data)
{

}

void testCallback2(int fd, int revents, void* data)
{

}

TEST(epoll_event_callback, init_destory)
{
    int epfd = epollCreate();
    
    EpvCallback* epvCallback = epvCallbackInit(STDIN_FILENO, EPOLLIN, testCallback1, epfd);
    ASSERT_TRUE(epvCallback != NULL);
    ASSERT_TRUE(epvCallback->epfd_ == epfd);
    ASSERT_TRUE(epvCallback->fd_ == STDIN_FILENO);
    ASSERT_TRUE(epvCallback->events_ == EPOLLIN);
    ASSERT_TRUE(epvCallback->callback_ == testCallback1);
    epvCallbackDestory(&epvCallback);
    ASSERT_TRUE(epvCallback == NULL);
    close(epfd);
}

TEST(epoll_event_callback, epollAdd)
{
    int epfd = epollCreate();
    int pipefd[2];
    pipe(pipefd);
    int writeFd = pipefd[1];
    int readFd = pipefd[0];
    EpvCallback* epvCallback = epvCallbackInit(readFd, EPOLLIN, testCallback1, epfd);
    epollAdd((void*)epvCallback);

    const char* msg = "hello world";
    int n = write(writeFd, msg, strlen(msg));
    ASSERT_TRUE(n == 11);
    epoll_event epv;
    memset(&epv, 0x00, sizeof(epv));
    n = epoll_wait(epfd, &epv, 1, 1000);
    ASSERT_TRUE(n == 1);
    EpvCallback* rEpvCallback = (EpvCallback*)epv.data.ptr;
    char buf[1024];
    memset(buf, 0x00, sizeof(buf));
    n = read(rEpvCallback->fd_, buf, sizeof(buf));
    ASSERT_TRUE(n == strlen(msg));
    ASSERT_TRUE(strcmp(msg, buf) == 0);
    epvCallbackDestory(&epvCallback);
    close(epfd);
    close(writeFd);
    close(readFd);
}

TEST(epoll_event_callback, epollMod)
{
    int epfd = epollCreate();
    int pipefd[2];
    pipe(pipefd);
    int writeFd = pipefd[1];
    int readFd = pipefd[0];
    EpvCallback* epvCallback = epvCallbackInit(writeFd, EPOLLIN, testCallback1, epfd);
    epollAdd((void*)epvCallback);
    epoll_event epv;
    int n = epoll_wait(epfd, &epv, 1, 1000);
    ASSERT_TRUE(n == 0);
    epvCallback->fd_ = writeFd;
    epvCallback->events_ = EPOLLOUT;
    epvCallback->callback_ = testCallback2;
    epollMod((void*)epvCallback);
    write(writeFd, "hello", 5);
    n = epoll_wait(epfd, &epv, 1, 1000);
    ASSERT_TRUE(n == 1);
    EpvCallback* rEpvCallback = (EpvCallback*)epv.data.ptr;
    ASSERT_TRUE(rEpvCallback->events_ == EPOLLOUT);
    ASSERT_TRUE(rEpvCallback->callback_ == testCallback2);
    epvCallbackDestory(&epvCallback);
    close(epfd);
    close(readFd);
}

TEST(epoll_event_callback, epollDel)
{
    int epfd = epollCreate();
    int pipefd[2];
    pipe(pipefd);
    int writeFd = pipefd[1];
    int readFd = pipefd[0];
    EpvCallback* epvCallback = epvCallbackInit(readFd, EPOLLIN, testCallback1, epfd);
    epollAdd((void*)epvCallback);

    write(writeFd, "hello", 5);
    epoll_event epv;
    memset(&epv, 0x00, sizeof(epv));
    int n = epoll_wait(epfd, &epv, 1, 1000);
    ASSERT_TRUE(n == 1);
    epollDel(epv.data.ptr);

    write(writeFd, "hello", 5);
    n = epoll_wait(epfd, &epv, 1, 1000);
    ASSERT_TRUE(n == 0);
    epvCallbackDestory(&epvCallback);
    close(epfd);
    close(writeFd);
    close(readFd);
}


