#ifndef EPOLL_EVENT_CALLBACK
#define EPOLL_EVENT_CALLBACK
#include <stddef.h>         // NULL
#include <stdio.h>

// 先前声明
struct Buffer;
struct EpollThreadPoll;


// 事件回调结构体
struct EpvCallback
{
    // 被监听的文件描述符
    int fd_;
    // 监听的事件类型
    int events_;
    // epoll触发的反应堆函数
    void(*callback_)(int fd, int events, void* data);
    // 反应堆去触发的函数：要做的业务逻辑
    void(*func_)(char* clientMsg, ssize_t clientMsgLen, Buffer* output);
    // 对应线程的epoll树根节点
    int epfd_;
    // 接受缓冲区
    Buffer* inputBuf_;
    // 发送缓冲区
    Buffer* outputBuf_;
    // 线程池指针
    EpollThreadPoll* epollThreadPoll_;
};

int epollCreate();

void epollAdd(void* data);

void epollMod(void* data);

void epollDel(void* data);

EpvCallback* epvCallbackInit(int fd, 
                            int events, 
                            void(*callback)(int fd, int revents, void*data), 
                            int epfd, 
                            EpollThreadPoll* epollThreadPoll = NULL, 
                            void(*func)(char* clientMsg, ssize_t clientMsgLen, Buffer* output) = NULL
                            );

void epvCallbackDestory(EpvCallback** epvCallback);


#endif