#ifndef EPOLL_EVENT_CALLBACK
#define EPOLL_EVENT_CALLBACK
#include <stddef.h>         // NULL

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
    // 发生变化后我们要做的业务逻辑
    void(*callback_)(int fd, int events, void* data);
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
                            EpollThreadPoll* epollThreadPoll = NULL);

void epvCallbackDestory(EpvCallback** epvCallback);


#endif