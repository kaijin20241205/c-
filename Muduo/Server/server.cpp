#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>              // inet_pton
#include <netinet/in.h>             // sockaddr_in
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>                 // read、write
#include <pthread.h>
#include <cstring>                  // memset
#include <sys/eventfd.h>
#include <netinet/tcp.h>            // TCP_NODELAY
#include <sys/uio.h>                // readv
using namespace std;



// 发生错误时打印错误信息、代码行号并退出程序
void errorExit(bool errorFlag, const char* errMsg, int errLine)
{
    if (errorFlag == true)
    {
        fprintf(stderr, "Error %s at: %d", errMsg, errLine);
        exit(EXIT_FAILURE);
    }
}

// 发生错误时打印错误信息、代码行号
void errorPrint(bool errorFLag, const char* errMsg, int errLine)
{
    if (errorFLag == true)
    {
        fprintf(stderr, "Error %s at: %d", errMsg, errLine);
    }
}








// void echoServer(char* data, int dataLen, string* sendMsg)
// {
//     copy(data, data + dataLen, sendMsg->data());
// }







// 向前声明
void writer(int cfd, int revents, void* data);
void reader(int cfd, int revents, void* data);
// void writer(int cfd, int revents, void* data)
// {
//     EpvCallback* epvCallback = reinterpret_cast<EpvCallback*>(data);
//     if (revents & EPOLLOUT && epvCallback->events_ & EPOLLOUT)
//     {
//         // 发送剩余数据
//         cout << "subthread: " << pthread_self() << "writer function working..." << endl;
//         int n = write(cfd, epvCallback->buf_, epvCallback->readWriteIndex_);
//         // 成功将剩余数据发送，让epoll监听EPOLLIN事件
//         if (n == epvCallback->readWriteIndex_)
//         {
//             epvCallback->readWriteIndex_ = 0;
//             memset(epvCallback->buf_, 0x00, epvCallback->bufLen_);
//             epvCallback->callback_ = reader;
//             epvCallback->events_ = EPOLLIN;
//             epoll_event epv{};
//             epv.events = EPOLLIN;
//             epv.data.ptr = data;
//             epoll_ctl(epvCallback->epfd_, EPOLL_CTL_MOD, cfd, &epv);
//         }
//         // 发送的过程中内核的发送缓冲区满了,没办法发送了,继续等待epoll_wait通知调用writer将剩余的内容发送
//         else if (0 < n < epvCallback->readWriteIndex_)
//         {
//             string tempStr(epvCallback->buf_ + n, epvCallback->buf_ + epvCallback->readWriteIndex_);
//             memset(epvCallback->buf_, 0x00, epvCallback->bufLen_);
//             copy(tempStr.begin(), tempStr.end(), epvCallback->buf_);
//             epvCallback->readWriteIndex_ -= n;
//         }
//         // 发生错误或者对端关闭了
//         else
//         {
//             if (errno != EAGAIN && errno != EWOULDBLOCK)
//             {
//                 epoll_ctl(epvCallback->epfd_, EPOLL_CTL_DEL, cfd, NULL);
//                 delete[] epvCallback->buf_;
//                 delete epvCallback;
//                 close(cfd);
//             }
//         }
//     }
// }

void reader(int cfd, int revents, void* data)
{
    // cout << "subthread: " << pthread_self() << "reader function working..." << endl;
    EpvCallback* epvCallbackEvent = reinterpret_cast<EpvCallback*>(data);
    // 1、有消息可读-配合堆区内存使用readv一次将数据都读完
    cout << "reader..." << endl;
    if (revents & EPOLLIN && epvCallbackEvent->events_ & EPOLLIN)
    {
        // const int enableWriteSize = epvCallbackEvent->inputBufLen_ - epvCallbackEvent->inputIndex_;
        // const int iovcnt = enableWriteSize < 65536 ? 2 : 1;
        // // 1.1、创建一个堆区变量一起作为缓冲区
        // char tempBuf[65536]{0};
        // iovec vec[2];
        // vec[0].iov_len = enableWriteSize;
        // vec[0].iov_base = epvCallbackEvent->inputBuf_ + epvCallbackEvent->inputIndex_;
        // vec[1].iov_len = sizeof(tempBuf);
        // vec[1].iov_base = tempBuf;
        // const ssize_t n = readv(epvCallbackEvent->fd_, vec, iovcnt);
        // // readv发生错误或者对端断开连接
        // if (n <= 0)
        // {
        //     cout << "close: " << cfd << endl;
        //     epoll_ctl(epvCallbackEvent->epfd_, EPOLL_CTL_DEL, cfd, NULL);
        //     close(cfd);
        //     // 释放EpvCallback在堆区的内存
        //     delete[] epvCallbackEvent->inputBuf_;
        //     delete[] epvCallbackEvent->outputBuf_;
        //     delete epvCallbackEvent;
        //     return;
        // }
        // // 1.2、只用到事件回调函数自带的缓冲区
        // else if (n <= enableWriteSize)
        // {
        //     epvCallbackEvent->inputIndex_ += n;
        // }
        // // 1.3、用到了栈区的缓冲区，一个socket（UDP）包最大只有65535-8（首部）=65527
        // else
        // {
        //     int tempBufLen = n - enableWriteSize;
        //     char* newBuf = new char[epvCallbackEvent->inputBufLen_ + tempBufLen];
        //     copy(epvCallbackEvent->inputBuf_, 
        //         epvCallbackEvent->inputBuf_ + epvCallbackEvent->inputBufLen_, 
        //         newBuf);
        //     copy(tempBuf, tempBuf + tempBufLen, newBuf + epvCallbackEvent->inputBufLen_);

        //     delete[] epvCallbackEvent->inputBuf_;
        //     epvCallbackEvent->inputBuf_ = newBuf;
        //     epvCallbackEvent->inputBufLen_ = epvCallbackEvent->inputBufLen_ + tempBufLen;
        //     epvCallbackEvent->inputIndex_ = epvCallbackEvent->inputBufLen_ + tempBufLen;
        // }
        // int enableWrite = epvCallbackEvent->outputBufLen_ - epvCallbackEvent->outputIndex_;
        // // 1.4、如果发送缓冲区可写数据小于要
        // if (enableWrite < epvCallbackEvent->inputIndex_)
        // {
        //     delete
        // }
        // else
        // {
        //     copy(epvCallbackEvent->inputBuf_, 
        //             epvCallbackEvent->inputBuf_ + epvCallbackEvent->inputIndex_, 
        //             epvCallbackEvent->outputBuf_ + epvCallbackEvent->outputIndex_);
        // }
        // int n = write(epvCallbackEvent->fd_, 
        //                 epvCallbackEvent->inputBuf_, 
        //                 epvCallbackEvent->inputBufLen_);
        // // 发送一部分
        // if (0 < n < epvCallbackEvent->inputBufLen_)
        // {
        //     int maining = epvCallbackEvent->inputBufLen_ - n;
        //     if (maining > epvCallbackEvent->outputBufLen_)
        //     {
        //     }
        // }
    }
}

void acceptor(int lfd, int revents, void* data)
{
    // 1、获取监听套接字绑定事件回调结构体对象
    EpvCallback* lfdEpvCallback = reinterpret_cast<EpvCallback*>(data);
    // 2、确定epoll返回的发送变化的监听事件与我们原本给epoll监听的事件一致，都是读事件，说明有客户端连接到了
    if (revents & EPOLLIN && lfdEpvCallback->events_ & EPOLLIN)
    {
        // 2.1、调用accept4从操作系统为监听套接字分配的的已完成连接队列中取出一个链接，获得与该链接通信的通信套接字cfd
        int cfd = accept4(lfd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        // accept4调用成功
        if (cfd > 0)
        {
            cout << "mainthread: " << pthread_self() << "hello cfd:" << cfd << endl;
            EpvCallback* cfdEpvCallback = initEpvCallback(cfd, EPOLLIN, reader, lfdEpvCallback->epollThreadPoll_->getNextSubThtreadEpfd());
            epollAdd(reinterpret_cast<void*>(cfdEpvCallback));
        }
        // 调用accept4发送时发送错误
        else
        {
            // error：调用accept4时已经没有客户端在进行连接请求
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                cout << "EAGAIN || EWOULDBLOCK" << endl;
            }
            // error：发生其他错误导致accept4无法接受新的客户端链接
            else
            {
                errorPrint(
                    true,
                    "accept4 error", 
                    __LINE__
                );
            }
        }
    }
    
    

}

// 创建套接字、绑定ip、port并设置监听状态
int initListenBindNonblock()
{
    // 1、创建套接字,使用IPV4，流协议、非阻塞、tcp协议
    int lfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    errorExit(lfd < 0, "socket error", __LINE__);
    // 1.1、设置ip地址端口复用
    int optval = 1;
    errorPrint(
        setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1, 
        "setsockopt SO_REUSEADDR error", 
        __LINE__
    );
    // 1.2、禁用nagle算法，使服务端发送的数据包尽快到达客户端
    errorPrint(
        setsockopt(lfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) == -1, 
        "setsockopt TCP_NODELAY error",
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
        __LINE__
    );
    // 2.4、设置协议IPV4
    localAddr.sin_family = AF_INET;
    socklen_t len = sizeof(localAddr);
    // 2.5、调用bind函数进行绑定
    errorExit(
        bind(lfd, (sockaddr*)&localAddr, len) == -1, 
        "bind error", 
        __LINE__
    );

    // 3、将套接字设置为监听状态
    errorExit(
        listen(lfd, 128) == -1, 
        "listen error",
        __LINE__
    );

    return lfd;
}

int main() 
{   
    int lfd = initListenBindNonblock();
   
    EpollThreadPoll* epollThreadPoll = epollThreadPollInit(1);

    epoll_event lfdEpv{};

    lfdEpv.events = EPOLLIN;

    EpvCallback epvCallbackEvent{};
    memset(&epvCallbackEvent, 0x00, sizeof(EpvCallback));

    epvCallbackEvent.fd_ = lfd;

    epvCallbackEvent.events_ = lfdEpv.events;

    epvCallbackEvent.callback_ = acceptor;

    epvCallbackEvent.epfd_ = epollThreadPoll->mainThreadEpfd;

    epvCallbackEvent.epollThreadPoll_ = epollThreadPoll;

    lfdEpv.data.ptr = reinterpret_cast<void*>(&epvCallbackEvent);
    
    errorExit(
        epoll_ctl(epvCallbackEvent.epfd_, EPOLL_CTL_ADD, lfd, &lfdEpv), 
        "epoll_ctl lfd error", 
        __LINE__
    );

    vector<epoll_event> epvs(16);
    int epollWaitSecondMs = 10000;

    while (true)
    {
        int num = epoll_wait(epvCallbackEvent.epfd_, &*epvs.begin(), epvs.size(), epollWaitSecondMs);
        if (num > 0)
        {
            auto it = epvs.begin();
            for (int i = 0; i < num; i++, it++)
            {
                EpvCallback* epvCallback = reinterpret_cast<EpvCallback*>(it->data.ptr);
                if (epvCallback->fd_ == lfd)
                {
                    epvCallback->callback_(epvCallback->fd_, it->events, it->data.ptr);
                }
            }
        }
        else if (num == 0)
        {
            cout << "mainthread: " << pthread_self() << " epoll_wait timeout..." << endl;
        }
        else
        {
            errorExit(
                errno != EINTR, 
                "epoll_wait error", 
                __LINE__
            );
        } 
    }

    threadPollDestory(epollThreadPoll);
    close(lfd);
    close(epvCallbackEvent.epfd_);
    return 0;
}