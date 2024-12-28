#include "./../include/CallbackFunction.h"
#include "./../include/EpollThreadPoll.h"
#include "./../include/EpollEventCallback.h"
#include "./../include/MyError.h"
#include "./../include/Buffer.h"


#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>          // sockaddr_in、
#include <string.h>             // memset
#include <errno.h>
// #include <unistd.h>
#include <stdlib.h>             // free 


void writer(int cfd, int revents, void* data)
{
    EpvCallback* epvCallbackEvent = (EpvCallback*)data;
    if (revents & EPOLLOUT && epvCallbackEvent->events_ & EPOLLOUT)
    {
        // 发送剩余数据
        int errorNum = 0;
        ssize_t outputMsgLen = bufferReadableBytes(epvCallbackEvent->outputBuf_);
        size_t n = bufferWriteFd(cfd, epvCallbackEvent->outputBuf_, &errorNum);
        if (n == -1)
        {
            if (errorNum != EAGAIN && errorNum != EWOULDBLOCK)
            {
                epollDel((void*)epvCallbackEvent);
                epvCallbackDestory(&epvCallbackEvent);
            }
        }
        else
        {
            // 全部发送了，切换成EPOLLIN
            if (n == outputMsgLen)
            {
                epvCallbackEvent->events_ = EPOLLIN;
                epvCallbackEvent->callback_ = reader;
                epollMod(data);
            }
        }
    }
}

void reader(int cfd, int revents, void* data)
{
    EpvCallback* epvCallbackEvent = (EpvCallback*)data;
    // 有消息可读
    if (revents & EPOLLIN && epvCallbackEvent->events_ & EPOLLIN)
    {
        int errorNum = 0;
        size_t n = bufferReadFd(cfd, epvCallbackEvent->inputBuf_, &errorNum);
        // readv发生错误或者对端断开连接
        if (n == -1)
        {
            if (errorNum != EAGAIN && errorNum != EWOULDBLOCK)
            {
                epollDel((void*)epvCallbackEvent);
                epvCallbackDestory(&epvCallbackEvent);
                printf("client disconnected...\n");
            }
        }
        // 没有发生错误
        else
        {
            // 如果存在用户自定义的函数
            if (epvCallbackEvent->func_ != NULL)
            {
                ssize_t clientMessageLen = bufferReadableBytes(epvCallbackEvent->inputBuf_);
                char* clientMssage = bufferRetrieveAllAsString(epvCallbackEvent->inputBuf_);
                epvCallbackEvent->func_(clientMssage, clientMessageLen, epvCallbackEvent->outputBuf_, epvCallbackEvent->data_);
                free(clientMssage);
                ssize_t outputMsgLen = bufferReadableBytes(epvCallbackEvent->outputBuf_);
                n = bufferWriteFd(cfd, epvCallbackEvent->outputBuf_, &errorNum);
                // write发生错误或者对方已断开连接
                if (n == -1)
                {
                    if (errorNum != EAGAIN && errorNum != EWOULDBLOCK)
                    {
                        epollDel((void*)epvCallbackEvent);
                        epvCallbackDestory(&epvCallbackEvent);
                    }
                }
                else
                {
                    // 只发送了一部分，还有一部分没发送，切换成EPOLLOUT委托epoll发送
                    if (n < outputMsgLen)
                    {
                        epvCallbackEvent->callback_ = writer;
                        epvCallbackEvent->events_ = EPOLLOUT;
                        epollMod(data);
                    }
                }
            }
            // 不存在用户自定义函数就只打印客户端发来的消息
            else
            {
                char* clientMssage = bufferRetrieveAllAsString(epvCallbackEvent->inputBuf_);
                printf("%s\n", clientMssage);
                free(clientMssage);
            }
            
        }
    }
}

void acceptor(int lfd, int revents, void* data)
{
    // 获取监听套接字绑定事件回调结构体对象
    EpvCallback* lfdEpvCallback = (EpvCallback*)data;
    // 确定epoll返回的发送变化的监听事件与我们原本给epoll监听的事件一致，都是读事件，说明有客户端连接到了
    if (revents & EPOLLIN && lfdEpvCallback->events_ & EPOLLIN)
    {
        sockaddr_in addr;
        memset(&addr, 0x00, sizeof(sockaddr_in));
        socklen_t addrLen = sizeof(sockaddr_in);
        // 调用accept4从操作系统为监听套接字分配的的已完成连接队列中取出一个链接，获得与该链接通信的通信套接字cfd
        int cfd = accept4(lfd, (sockaddr*)&addr, &addrLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
        // accept4调用成功
        if (cfd > 0)
        {
            char ip[16];
            memset(ip, 0x00, sizeof(ip));
            printf("client ip: %s, port: %d cfd: %d\n", 
                    inet_ntop(AF_INET, &(addr.sin_addr.s_addr), ip, sizeof(ip)), 
                    ntohs(addr.sin_port), 
                    cfd);
            int epfd = getNextSubThtreadEpfd(lfdEpvCallback->epollThreadPoll_);
            EpvCallback* cfdEpvCallback = epvCallbackInit(cfd, 
                                                            EPOLLIN, 
                                                            reader, 
                                                            epfd, 
                                                            NULL, 
                                                            lfdEpvCallback->func_);

            
            epollAdd((void*)cfdEpvCallback);
        }
        // 调用accept4发送时发送错误
        else
        {
            // error：调用accept4时已经没有客户端在进行连接请求
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                errorPrint(
                    true, 
                    "accept4 EAGAIN || EWOULDBLOCK", 
                    __FILE__, 
                    __LINE__
                );
            }
            // error：发生其他错误导致accept4无法接受新的客户端链接
            else
            {
                errorPrint(
                    true,
                    "accept4 error", 
                    __FILE__, 
                    __LINE__
                );
            }
        }
    }
}




