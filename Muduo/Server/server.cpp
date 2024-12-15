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


struct Buffer
{

    // 缓冲区大小
    int bufLen_;
    // 读指针缓冲区指针
    int readIndex_;
    // 写指针缓冲区指针
    int writeIndex_;
};

struct EpollThreadPoll
{
    //  线程池中的子线程数量
    int threadNum_;
    // 线程id数组用来存储所有线程的线程id
    pthread_t* threads_;
    // 销毁线程池标志
    bool destoryFlag;
    // int类型的数组指针，保存所有子线程的epfd
    int* subThtreadEpfds_;
    // subThtreadEpfds_数组的下标索引，用来轮询subThtreadEpfds_数组用的
    uint64_t subThreadEpfdsIndex_;
    // 主线程的epfd，如果子线程数量=0，那么getNextSubThtreadEpfd返回的就是主线程的epfd
    int mainThreadEpfd;
    // 轮询subThtreadEpfds_数组获得一个epfd
    int getNextSubThtreadEpfd()
    {
        if (threadNum_ < 1)
        {
            return mainThreadEpfd;
        }
        // 通过取余操作来不停轮询
        return subThtreadEpfds_[subThreadEpfdsIndex_++ % threadNum_];
    }
    // 主、子线程用来通信的文件描述符，来控制按照次序创建子线程
    int evnetFd_;
};

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

void epollAdd(void* data)
{
    EpvCallback* epvCallback = reinterpret_cast<EpvCallback*>(data);
    epoll_event epv{0};
    epv.events = epvCallback->events_;
    epv.data.ptr = data;
    errorExit(
        epoll_ctl(epvCallback->epfd_, EPOLL_CTL_ADD, epvCallback->fd_, &epv) == -1, 
        "epollAdd error", 
        __LINE__
    );
}

void epollMod(void* data)
{
    EpvCallback* epvCallback = reinterpret_cast<EpvCallback*>(data);
    epoll_event epv{0};
    epv.events = epvCallback->events_;
    epv.data.ptr = data;
    errorPrint(
        epoll_ctl(epvCallback->epfd_, EPOLL_CTL_MOD, epvCallback->fd_, &epv) == -1, 
        "epollMdd error", 
        __LINE__
    );
}

void epollDel(void* data)
{
    EpvCallback* epvCallback = reinterpret_cast<EpvCallback*>(data);
    errorPrint(
        epoll_ctl(epvCallback->epfd_, EPOLL_CTL_MOD, epvCallback->fd_, nullptr) == -1, 
        "epollMdd error", 
        __LINE__
    );
}




// 向前声明
struct EpollThreadPoll;






EpvCallback* initEpvCallback(int fd, 
                            int events, 
                            void(*callback)(int fd, int revents, void*data), 
                            int epfd) 
                            // EpollThreadPoll* epollThreadPoll)
{
    EpvCallback* epvCallback = new EpvCallback;
    errorExit(
        epvCallback == nullptr, 
        "initEpvCallback error", 
        __LINE__
    );
    memset(epvCallback, 0x00, sizeof(EpvCallback));
    epvCallback->fd_ = fd;
    epvCallback->events_ = events;
    epvCallback->callback_ = callback;
    epvCallback->epfd_ = epfd;
    epvCallback->epollThreadPoll_ = nullptr;
    // 应该是initBuffer
    // epvCallback->inputBuf_ = new Buffer;
    // memset(epvCallback->inputBuf_, 0x00, sizeof(Buffer));
    // epvCallback->outputBuf_ = new Buffer;
    // memset(epvCallback->outputBuf_, 0x00, sizeof(Buffer));

    return epvCallback;
}



// 子线程运行函数
void* threadWorkFunc(void* data)
{
    // 1、获取线程池对象
    EpollThreadPoll* epollThreadPoll = reinterpret_cast<EpollThreadPoll*>(data);
    cout << "No:" << epollThreadPoll->subThreadEpfdsIndex_ + 1 << " subthread: " << pthread_self() << "create begin..." << endl;
    // 2、创建子线程epoll
    int subEpfd = epoll_create1(EPOLL_CLOEXEC);
    // 3、准备好写入eventfd的数据
    // 数据值大于0表示事件状态设置为已发生，如果成功写入主线程的read函数会解除阻塞
    uint64_t message = 1;
    if (subEpfd < 0)
    {
        // 3.1、如果创建epfd失败,设置线程分离,通知主线程解除阻塞,创建下一个线程
        cout << "subthread:" << pthread_self() << " epoll_create1 error" << endl;
        // 创建epoll失败，让线程数-1，
        epollThreadPoll->threadNum_--;
        // 设置线程分离，让子线程由1号进程来回收资源
        pthread_detach(pthread_self());
        // 通知主线程可以开始创建下一个子线程了
        write(epollThreadPoll->evnetFd_, &message, sizeof(message));
        return NULL;
    }
    
    // 3.2、创建epoll成功，将epfd按照子线程创建的顺序赋值给线程池的epollThreadPoll->subThtreadEpfds_数组中
    uint64_t epfdIndex = epollThreadPoll->subThreadEpfdsIndex_++;
    epollThreadPoll->subThtreadEpfds_[epfdIndex] = subEpfd;

    // 3.3、创建子线程再epoll_wait中用来接收epoll_evnet的数组
    vector<epoll_event> epvs(16);
    // 3.4、设置子线程的超时等待事件为10000毫秒
    int subEpollWaitSecondMs = 10000;
    
    cout << "No:" << epollThreadPoll->subThreadEpfdsIndex_ << " subthread: " << pthread_self() << "in epoll_wait" << endl;
    // 3.5、子线程准备工作完成，通知主线程可以创建下一个子线程了
    write(epollThreadPoll->evnetFd_, &message, sizeof(message));
    // 3.6、当线程池不处于摧毁状态时，子线程进入epoll_wait循环
    while (!epollThreadPoll->destoryFlag)
    {
        // 如果主线程未向子线程的epfd添加通信套接字，所以子线程一直等待、苏醒、等待、苏醒....
        int num = epoll_wait(subEpfd, &*epvs.begin(), epvs.size(), subEpollWaitSecondMs);
        if (num > 0)
        {
            // 3.7、依次获取epoll返回的epoll_event事件
            auto it = epvs.begin();
            for (int i = 0; i < num; i++, it++)
            {
                // 3.8、获得自定义的epoll事件回调结构体
                EpvCallback* evpCallback = reinterpret_cast<EpvCallback*>(it->data.ptr);
                // 3.9、通过事件回调结构体中的回调函数来处理客户端的读、写事件
                // 子线程中的evpCallback->callback_都是reader或者writer
                evpCallback->callback_(evpCallback->fd_, it->events, it->data.ptr);
            }
        }
        else if (num == 0)
        {
            cout << "subthread " << pthread_self() << ": epoll_wait timeout..." << endl;
        }
        else
        {
            // epoll_wait发生错误并且不是因为被信号打断，就打印错误消息
            if (errno != EINTR)
            {
                cout << "subthread " << pthread_self() << " epoll_wait error, errno:" << errno << endl;
                epollThreadPoll->subThtreadEpfds_[epfdIndex] = -1;
                break;
            }
        } 
    }
    // 4.0、子线程准备退出，关闭epfd
    close(subEpfd);
    return NULL;
}


EpollThreadPoll* epollThreadPollInit(int threadNum)
{
    /*
        设计一个管理线程的线程池结构体，
        主要作用：
        1、主线程能通过通过该结构体对象获得子线程的epoll树根节点
        2、主线程能通过通过该结构体对象让子线程退出工作函数
        3、主线程能够依次创建子线程
    */
    
    // 1、 创建一个线程池结构体对象，生命周期要和程序一致,所以使用堆区内存
    EpollThreadPoll* epollThreadPoll = new EpollThreadPoll;
    errorExit(
        epollThreadPoll == nullptr,
        "new ThreadPoll error",
        __LINE__
    );
    // 主线程创建epoll
    epollThreadPoll->mainThreadEpfd = epoll_create1(EPOLL_CLOEXEC);
    errorExit(
        epollThreadPoll->mainThreadEpfd == -1, 
        "epoll_create1 error",
        __LINE__
    );

    // 2.1 如果参数threadNum小于1，直接返回不需要做初始化操作
    if (threadNum < 1)
    {
        return epollThreadPoll;
    }
    // 2.2、threadNum >= 1，对线程池属性进行初始化操作
    memset(epollThreadPoll, 0x00, sizeof(EpollThreadPoll));
    // 设置线程数量
    epollThreadPoll->threadNum_ = threadNum;
    // 线程池是否要摧毁
    epollThreadPoll->destoryFlag = false;
    // 线程id数组
    epollThreadPoll->threads_ = new pthread_t[threadNum];
    errorExit(
        epollThreadPoll->threads_ == nullptr,
        "new pthread_t[] error",
        __LINE__
    );
    // 初始化线程id数组
    memset(epollThreadPoll->threads_, 0x00, sizeof(pthread_t) * threadNum);
    // 存储所有子线程的epfd的数组
    epollThreadPoll->subThtreadEpfds_ = new int[threadNum];
    errorExit(
        epollThreadPoll->subThtreadEpfds_ == nullptr,
        "new int[threadNum] error", 
        __LINE__
    );
    // 初始化
    memset(epollThreadPoll->subThtreadEpfds_, 0x00, sizeof(int) * threadNum);
    // 初始化用来获取子线程epfd的下标索引
    epollThreadPoll->subThreadEpfdsIndex_ = 0;

    // 3、创建一个eventFd让子线程的准备工作完成后（创建到epoll_wait时）再通知主线程创建下一个子线程
    // 3.1、eventfd初始化时设置，事件状态设置为未发生，为阻塞状态，如果没有消息发来就一直阻塞
    epollThreadPoll->evnetFd_ = eventfd(0, 0);
    errorExit(
        epollThreadPoll->evnetFd_ == -1,
        "eventfd error",
        __LINE__
    );
    uint64_t message = 0;
    
    // 4、根据threadNum数量创建子线程
    for (int i = 0; i < threadNum; i++)
    {
        // 4.1、创建子线程成功，进入threadWorkFunc子线程函数执行子线程代码
        int ret = pthread_create(&epollThreadPoll->threads_[i], 
                                NULL, 
                                threadWorkFunc, 
                                reinterpret_cast<void*>(epollThreadPoll));
        // 4.2、创建子线程线程失败，线程池的子线程数量减一
        if (ret != 0)
        {
            epollThreadPoll->threadNum_--;
        }
        // 4.3、创建线程成功，主线程需要等待子线程执行到特定代码再继续创建下一个子线程
        else
        {
            // 4.4、如果read阻塞时被信号打断就重试
            while (read(epollThreadPoll->evnetFd_, &message, sizeof(message)) == -1 && errno == EINTR)
            {
            }
        }
    }
    // 5、子线程创建完毕，不需要eventfd、关闭掉
    close(epollThreadPoll->evnetFd_);
    epollThreadPoll->evnetFd_ = -1;
    // 6、将线程池对象返回
    return epollThreadPoll;
}

void threadPollDestory(EpollThreadPoll* epollThreadPoll)
{
    if (epollThreadPoll != nullptr)
    {
        // 回收子线程资源
        for (int i = 0; i < epollThreadPoll->threadNum_; i++)
        {
            if (epollThreadPoll->threads_[i] != 0)
            {
                pthread_join(epollThreadPoll->threads_[i], NULL);
            }
        }
        delete[] epollThreadPoll->threads_;
        delete[] epollThreadPoll->subThtreadEpfds_;
        delete epollThreadPoll;
    }
    cout << "threadPoll deleted..." << endl;
}

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
    // 一、创建非阻塞套接字、为套接字绑定ip、port、将套接字设置为监听套接字
    int lfd = initListenBindNonblock();

    // 二、创建子线程，让子线程创建属于自己的epoll并进入epoll_wait，等待主线程给子线程的epoll分配通信套接字
    EpollThreadPoll* epollThreadPoll = epollThreadPollInit(1);
    
    // 三、为监听套接字绑定epoll_evnet，为监听套接字的epoll_evnet绑定事件回调对象并启动epoll_wait

    /*
        1、给线程池中的mainThreadEpfd赋值，当线程池的子线程数组小于1时，调用getNextSubThtreadEpfd函数
        返回的就是主线程的epfd
    */

    // 2、给监听套接字创建epoll_event
    epoll_event lfdEpv{};
    // 3、设置epoll_event的事件属性，让epoll监听EPOLLIN事件
    lfdEpv.events = EPOLLIN;
    /*

        4、要使用到epoll_event.data.ptr这个属性来实现epoll反应堆，就先要设计一个自定义的事件回调结构体
        结构体必须的属性：
            1、套接字：（监听套接字、通信套接字）主线程拥有监听套接字，主线程调用accept获得通信套接字后分配给子线程、
            子线程通过通信套接字读或者写
            2、需要监听套接字的事件
            3、套接字监听的事件发生后要调用的回调函数：主线程只触发acceptor回调，子线程触发reader、writer回调
            4、epoll树根节点（epfd），用来将监听套接字、epoll_event事件绑定到主线程的epoll上，将通信套接字、epoll_event
            绑定到子线程的epoll上
            5、读写缓冲区（堆区内存，每个套接字都拥有自己的一份读写读写缓冲区），子线程读写时用的，设置合理的缓冲区大小可以
            用来减少read、write系统调用
            6、读写缓冲区的长度
            7、读写指针，表示当前读或者写到什么位置
            8、线程池指针：只给主线程的通信套接字使用的，通过该指针的对象获取到子线程的epfd，完成通信套接字的分配，如果
            acceptor实现在main函数中可以删除这个属性
    */
    // 5、创建监听套接字的事件回调结构体对象，这个对象在main函数中，生命周期与主线程一样
    // 并且也不需要使用到Buffer对象，所以没必要在堆区开辟
    EpvCallback epvCallbackEvent{};
    memset(&epvCallbackEvent, 0x00, sizeof(EpvCallback));
    // 6、设置监听套接字的事件回调结构体属性：
    epvCallbackEvent.fd_ = lfd;
    // 设置监听套接字的要让epoll监听的事件是什么
    epvCallbackEvent.events_ = lfdEpv.events;
    // 设置监听套接字监听的事件发生变化后要调用的回调
    epvCallbackEvent.callback_ = acceptor;
    // 设置监听套接字所属的epoll
    epvCallbackEvent.epfd_ = epollThreadPoll->mainThreadEpfd;
    // 设置一下线程池对象
    epvCallbackEvent.epollThreadPoll_ = epollThreadPoll;

    // 7、将监听套接字绑定的epoll_evnet中的data.ptr指针指向监听套接字回调结构体对象，完成epoll_event与事件回调结构体对象的绑定
    lfdEpv.data.ptr = reinterpret_cast<void*>(&epvCallbackEvent);
    
    // 8、将监听套接字绑定的epoll_evnet添加到epoll
    errorExit(
        epoll_ctl(epvCallbackEvent.epfd_, EPOLL_CTL_ADD, lfd, &lfdEpv), 
        "epoll_ctl lfd error", 
        __LINE__
    );

    // 9、创建一个epoll_event类型的动态数组用来接收epoll监听的返回事件
    // 初始大小是16
    vector<epoll_event> epvs(16);
    // epoll_wait的超时等待时间设置为10000毫秒
    int epollWaitSecondMs = 10000;

    // 10、启动主线程的epoll_wait
    while (true)
    {
        int num = epoll_wait(epvCallbackEvent.epfd_, &*epvs.begin(), epvs.size(), epollWaitSecondMs);
        // 11、监听套接字的有可读事件了（有客户端连接了）
        if (num > 0)
        {
            auto it = epvs.begin();
            for (int i = 0; i < num; i++, it++)
            {
                EpvCallback* epvCallback = reinterpret_cast<EpvCallback*>(it->data.ptr);
                if (epvCallback->fd_ == lfd)
                {
                    // 在reactors in threads模型中（子线程数量大于等于1）主线程中的epvCallback->callback_都是acceptor函数
                    /*
                        四、调用acceptor回调函数，大概做下面4件事情
                        1、通过accept4获取通信套接字
                        2、将通信套接字创建并绑定epoll_event结构体对象
                        3、为通信套接字的epoll_event结构体对象创建并绑定事件回调结构体对象
                        4、将通信套接字、通信套接字绑定的epoll_evnet分配给子线程的epoll
                    */
                    epvCallback->callback_(epvCallback->fd_, it->events, it->data.ptr);
                }
            }
        }
        // 12、监听套接字在经过10000毫秒后仍然没有可读事件发生（没有客户端连接）
        else if (num == 0)
        {
            cout << "mainthread: " << pthread_self() << " epoll_wait timeout..." << endl;
        }
        // 13、epoll_wait发生错误
        else
        {
            // epoll_wait被信号打断就忽略否则就打印错误后退出程序
            errorExit(
                errno != EINTR, 
                "epoll_wait error", 
                __LINE__
            );
        } 
    }

    // 14、主线程退出
    // 销毁线程池
    threadPollDestory(epollThreadPoll);
    // 关闭监听文件描述符
    close(lfd);
    // 关闭主线程的epoll
    close(epvCallbackEvent.epfd_);
    return 0;
}