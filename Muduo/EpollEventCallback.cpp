#include "EpollEventCallback.h"
#include "MyError.h"
#include "Buffer.h"

#include <sys/epoll.h>
#include <string.h>         // memset
#include <stdlib.h>         // malloc、free
#include <unistd.h>


int epollCreate()
{
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    errorExit(
    epfd == -1, 
    "epollThreadPollInit epoll_create1 error", 
    __FILE__, 
    __LINE__
    );
    return epfd;
}

void epollAdd(void* data)
{
    EpvCallback* epvCallback = (EpvCallback*)data;
    epoll_event epv;
    memset(&epv, 0x00, sizeof(epoll_event));
    epv.events = epvCallback->events_;
    epv.data.ptr = data;
    errorPrint(
        epoll_ctl(epvCallback->epfd_, EPOLL_CTL_ADD, epvCallback->fd_, &epv) == -1, 
        "epollAdd error", 
        __FILE__, 
        __LINE__
    );
}

void epollMod(void* data)
{
    EpvCallback* epvCallback = (EpvCallback*)data;
    epoll_event epv;
    memset(&epv, 0x00, sizeof(epoll_event));
    epv.events = epvCallback->events_;
    epv.data.ptr = data;
    errorPrint(
        epoll_ctl(epvCallback->epfd_, EPOLL_CTL_MOD, epvCallback->fd_, &epv) == -1, 
        "epollMod error", 
        __FILE__, 
        __LINE__
    );
}

void epollDel(void* data)
{
    EpvCallback* epvCallback = (EpvCallback*)data;
    errorPrint(
        epoll_ctl(epvCallback->epfd_, EPOLL_CTL_DEL, epvCallback->fd_, NULL) == -1, 
        "epollDel error", 
        __FILE__, 
        __LINE__
    );
}

EpvCallback* epvCallbackInit(int fd, 
                            int events, 
                            void(*callback)(int fd, int revents, void*data), 
                            int epfd, 
                            EpollThreadPoll* epollThreadPoll, 
                            void(*func)(char* clientMsg, ssize_t clientMsgLen, Buffer* output)
                            )
{
    EpvCallback* epvCallback = (EpvCallback*)malloc(sizeof(EpvCallback));
    errorExit(
        epvCallback == NULL, 
        "epvCallbackInit error", 
        __FILE__, 
        __LINE__
    );
    memset(epvCallback, 0x00, sizeof(EpvCallback));
    epvCallback->fd_ = fd;
    epvCallback->events_ = events;
    epvCallback->callback_ = callback;
    epvCallback->epfd_ = epfd;
    epvCallback->epollThreadPoll_ = epollThreadPoll;
    epvCallback->func_ = func;
    epvCallback->inputBuf_ = bufferInit();
    epvCallback->outputBuf_ = bufferInit();

    return epvCallback;
}

void epvCallbackDestory(EpvCallback** epvCallback)
{
    if (epvCallback != NULL && *epvCallback != NULL)
    {
        if ((*epvCallback)->inputBuf_ != NULL)
        {
            bufferDestory(&((*epvCallback)->inputBuf_));
        }
        if ((*epvCallback)->outputBuf_ != NULL)
        {
            bufferDestory(&((*epvCallback)->outputBuf_));
        }
        close((*epvCallback)->fd_);
        free(*epvCallback);
        *epvCallback = NULL;
    }
}