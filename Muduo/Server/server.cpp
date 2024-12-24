#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <string.h>
#include <errno.h>

#include "MyError.h"
#include "EpollThreadPoll.h"
#include "EpollEventCallback.h"
#include "CallbackFunction.h"
#include "Buffer.h"


void echoServer(char *clientMsg, ssize_t clientMsgLen, Buffer *output, void* data)
{
    bufferAppend(clientMsg, clientMsgLen, output);
}


// 创建套接字、绑定ip、port并设置监听状态
int initListenBindNonblock()
{
    // 1、创建套接字,使用IPV4，流协议、非阻塞、tcp协议
    int lfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    errorExit(lfd < 0, "socket error", __FILE__, __LINE__);
    // 1.1、设置ip地址端口复用
    int optval = 1;
    errorPrint(
        setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1, 
        "setsockopt SO_REUSEADDR error", 
        __FILE__, 
        __LINE__
    );
    // 1.2、禁用nagle算法，使服务端发送的数据包尽快到达客户端
    errorPrint(
        setsockopt(lfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) == -1, 
        "setsockopt TCP_NODELAY error",
        __FILE__, 
        __LINE__
    );

    // 2、为套接字绑定ip、端口
    // 2.1、创建sockaddr_in并初始化
    sockaddr_in localAddr{};
    // 2.2、通过htons函数确保8888是网络字节序
    localAddr.sin_port = htons(8888);
    // 2.3、确保localAddr.sin_addr.s_addr是网络字节序
    errorExit(
        inet_pton(AF_INET, "127.0.0.1", &localAddr.sin_addr.s_addr) == -1,
        "inet_pton error", 
        __FILE__, 
        __LINE__
    );
    // 2.4、设置协议IPV4
    localAddr.sin_family = AF_INET;
    socklen_t len = sizeof(localAddr);
    // 2.5、调用bind函数进行绑定
    errorExit(
        bind(lfd, (sockaddr*)&localAddr, len) == -1, 
        "bind error", 
        __FILE__, 
        __LINE__
    );

    // 3、将套接字设置为监听状态
    errorExit(
        listen(lfd, 128) == -1, 
        "listen error",
        __FILE__, 
        __LINE__
    );

    return lfd;
}

int main() 
{   
    // 创建监听文件描述符
    int lfd = initListenBindNonblock();
    // 创建线程池
    EpollThreadPoll* epollThreadPoll = epollThreadPollInit(2);
    epollThreadPollStart(epollThreadPoll);
    // 给监听文件描述符创建事件回调结构体
    EpvCallback* lfdEpvCallback = epvCallbackInit(
                                    lfd, 
                                    EPOLLIN, 
                                    acceptor, 
                                    epollThreadPoll->mainThreadEpfd_, 
                                    epollThreadPoll, 
                                    echoServer);
    // 将监听回调结构体添加到epoll
    epollAdd((void*)lfdEpvCallback);

    int epvsSize = 16;
    epoll_event epvs[epvsSize];
    memset(epvs, 0x00, sizeof(epoll_event) * epvsSize);
    int epollWaitSecond = 10000;
    // 开始循环监听事件
    while (!epollThreadPoll->destoryFlag)
    {
        int n = epoll_wait(epollThreadPoll->mainThreadEpfd_, 
                    epvs, 
                    epvsSize, 
                    epollWaitSecond);
        if (n > 0)
        {
            for (int i = 0; i < n; i++)
            {
                EpvCallback* epvCallback = (EpvCallback*)epvs[i].data.ptr;
                epvCallback->callback_(epvCallback->fd_, epvCallback->events_, epvs[i].data.ptr);
            }
        }
        else if (n == 0)
        {
            printf("mainThread: %lu, epoll_wait timeout..\n", pthread_self());
        }
        else
        {
            if (errno != EINTR)
            {
                printf("mainThread: %lu, epoll_wait error, errno: errno\n", pthread_self());
                break;
            }
        }
    }
    epvCallbackDestory(&lfdEpvCallback);
    epollThreadPollDestory(&epollThreadPoll);
    return 0;
}