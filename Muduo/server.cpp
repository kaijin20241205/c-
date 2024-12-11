#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>              // inet_pton
#include <netinet/in.h>             // sockaddr_in
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include <cstring>
#include <sys/eventfd.h>
using namespace std;


struct ThreadPoll;

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
    // 读写缓冲区
    char* buf_;
    // 当前读写指针的位置
    int readWriteIndex_;
    // 读写缓冲区大小
    int bufLen_;
    // 线程池指针
    ThreadPoll* threadPoll_;
};


struct ThreadPoll
{
    //  线程数量
    int threadNum_;
    // 线程id数组
    pthread_t* threads_;
    // 销毁线程池标志
    bool destoryFlag;
    // 保存所有子线程的epfd的数组
    int* subThtreadEpfds_;
    // epfd数组的下标
    int subThreadEpfdsIndex_;
    // 主线程的epfd
    int mainThreadEpfd;
    // 轮询所有线程获得所属的epfd
    int getNextSubThtreadEpfd()
    {
        if (threadNum_ < 1)
        {
            return mainThreadEpfd;
        }
        return subThtreadEpfds_[subThreadEpfdsIndex_++ % threadNum_];
    }
    // 主、子线程通信的文件描述符
    int evnetFd_;
};


void* threadWorkFunc(void* myThreadPoll)
{
    ThreadPoll* threadPoll = reinterpret_cast<ThreadPoll*>(myThreadPoll);
    cout << "No:" << threadPoll->subThreadEpfdsIndex_ + 1 << " subthread: " << pthread_self() << "create begin..." << endl;
    int subEpfd = epoll_create1(EPOLL_CLOEXEC);
    // 事件状态设置为已发生
    uint64_t message = 1;
    if (subEpfd < 0)
    {
        // 如果创建epfd失败,设置线程分离,通知主线程解除阻塞,创建下一个线程
        cout << "subthread:" << pthread_self() << " epoll_create1 error" << endl;
        threadPoll->threadNum_--;
        pthread_detach(pthread_self());
        // 先通知再设置状态设置，如果先设置状态再发信号还是有可能被信号打断
        write(threadPoll->evnetFd_, &message, sizeof(message));
        return NULL;
    }
    
    // 创建epfd成功
    threadPoll->subThtreadEpfds_[threadPoll->subThreadEpfdsIndex_++] = subEpfd;

    vector<epoll_event> epvs(16);
    int subEpollWaitSecondMs = 10000;
    
    cout << "No:" << threadPoll->subThreadEpfdsIndex_ << " subthread: " << pthread_self() << "in epoll_wait" << endl;
    // 子线程准备工作完成，通知主线程可以创建下一个子线程了
    // 先通知再设置状态设置，如果先设置状态再发信号还是有可能被信号打断
    write(threadPoll->evnetFd_, &message, sizeof(message));
    while (!threadPoll->destoryFlag)
    {
        int num = epoll_wait(subEpfd, &*epvs.begin(), epvs.size(), subEpollWaitSecondMs);
        if (num > 0)
        {
            auto it = epvs.begin();
            for (int i = 0; i < num; i++, it++)
            {
                EpvCallback* evpCallback = reinterpret_cast<EpvCallback*>(it->data.ptr);
                if (it->events & EPOLLIN)
                {
                    evpCallback->callback_(evpCallback->fd_, it->events, it->data.ptr);
                }
            }
        }
        else if (num == 0)
        {
            cout << "subthread " << pthread_self() << ": epoll_wait timeout..." << endl;
        }
        else
        {
            // epoll_wait不是被信号打断
            if (errno != EINTR)
            {
                cout << "subthread epoll_wait error, errno:" << errno << endl;
                exit(EXIT_FAILURE);
            }
        } 
    }
    close(subEpfd);
    return NULL;
}

ThreadPoll* threadPollInit(int threadNum)
{
    // 线程池的生命周期要和程序一致,使用堆区内存
    ThreadPoll* myThreadPoll = new ThreadPoll;
    if (threadNum < 1)
    {
        return myThreadPoll;
    }
    // 初始化
    memset(myThreadPoll, 0x00, sizeof(ThreadPoll));
    // 设置线程数量
    myThreadPoll->threadNum_ = threadNum;
    // 线程池是否摧毁
    myThreadPoll->destoryFlag = false;
    // 线程id数组
    myThreadPoll->threads_ = new pthread_t[threadNum];
    // 初始化
    memset(myThreadPoll->threads_, 0x00, sizeof(pthread_t) * threadNum);
    // 存储所有工作线程的epfd数组
    myThreadPoll->subThtreadEpfds_ = new int[threadNum];
    // 初始化
    memset(myThreadPoll->subThtreadEpfds_, 0x00, sizeof(int) * threadNum);
    // 主线程用来获取子线程epfd的下标索引
    myThreadPoll->subThreadEpfdsIndex_ = 0;

    // 创建一个eventFd让子线程的准备工作完成后（创建到epoll_wait时）通知主线程创建下一个子线程
    myThreadPoll->evnetFd_ = eventfd(0, 0);
    if (myThreadPoll->evnetFd_ < 0)
    {
        cout << "threadpoll init fail, eventfd function error, errno: " << errno << endl;
        exit(EXIT_FAILURE);
    }
    // 事件状态设置为未发生，在read时阻塞等待
    uint64_t message = 0;
    for (int i = 0; i < threadNum; i++)
    {
        int ret = pthread_create(&myThreadPoll->threads_[i], 
                                NULL, 
                                threadWorkFunc, 
                                reinterpret_cast<void*>(myThreadPoll));
        // 创建线程失败
        if (ret != 0)
        {
            myThreadPoll->threadNum_--;
        }
        // 创建线程成功，主线程需要等待子线程执行到特定代码再继续创建下一个子线程
        else
        {
            // 如果被信号打断就重试
            while (read(myThreadPoll->evnetFd_, &message, sizeof(message)) == -1 && errno == EINTR)
            {
            }
        }
    }
    close(myThreadPoll->evnetFd_);
    myThreadPoll->evnetFd_ = -1;
    return myThreadPoll;
}

void threadPollDestory(ThreadPoll* threadPoll)
{
    if (threadPoll != nullptr)
    {

        // 回收子线程资源
        for (int i = 0; i < threadPoll->threadNum_; i++)
        {
            if (threadPoll->threads_[i] != 0)
            {
                pthread_join(threadPoll->threads_[i], NULL);
            }
        }
        delete[] threadPoll->threads_;
        delete[] threadPoll->subThtreadEpfds_;
        delete threadPoll;
    }
    cout << "threadPoll deleted..." << endl;
}

void reader(int cfd, int revents, void* data);
void writer(int cfd, int revents, void* data)
{
    EpvCallback* epvCallback = reinterpret_cast<EpvCallback*>(data);
    if (revents & EPOLLOUT && epvCallback->events_ & EPOLLOUT)
    {
        // 发送剩余数据
        cout << "subthread: " << pthread_self() << "writer function working..." << endl;
        int n = write(cfd, epvCallback->buf_, epvCallback->readWriteIndex_);
        // 成功将剩余数据发送，让epoll监听EPOLLIN事件
        if (n == epvCallback->readWriteIndex_)
        {
            epvCallback->readWriteIndex_ = 0;
            memset(epvCallback->buf_, 0x00, epvCallback->bufLen_);
            epvCallback->callback_ = reader;
            epvCallback->events_ = EPOLLIN;
            
            epoll_event epv{};
            epv.events = EPOLLIN;
            epv.data.ptr = data;
            epoll_ctl(epvCallback->epfd_, EPOLL_CTL_MOD, cfd, &epv);
        }
        // 发送的过程中内核的发送缓冲区满了,没办法发送了,继续等待epoll_wait通知调用writer将剩余的内容发送
        else if (0 < n < epvCallback->readWriteIndex_)
        {
            string tempStr(epvCallback->buf_ + n, epvCallback->buf_ + epvCallback->readWriteIndex_);
            memset(epvCallback->buf_, 0x00, epvCallback->bufLen_);
            copy(tempStr.begin(), tempStr.end(), epvCallback->buf_);
            epvCallback->readWriteIndex_ -= n;
        }
        // 发生错误或者对端关闭了
        else
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                epoll_ctl(epvCallback->epfd_, EPOLL_CTL_DEL, cfd, NULL);
                delete[] epvCallback->buf_;
                delete epvCallback;
                close(cfd);
            }
        }

    }
}


void reader(int cfd, int revents, void* data)
{
    cout << "subthread: " << pthread_self() << "reader function working..." << endl;
    EpvCallback* epvCallbackEvent = reinterpret_cast<EpvCallback*>(data);
    // 有消息可读
    if (revents & EPOLLIN && epvCallbackEvent->events_ & EPOLLIN)
    {
        cout << "cfd: " << cfd << "message: ";
        // 创建一个栈区的string用来存储客户端发来的消息
        string tempStr;
        while (!epvCallbackEvent->threadPoll_->destoryFlag)
        {
            memset(epvCallbackEvent->buf_, 0x00, epvCallbackEvent->bufLen_);
            int n = read(cfd, epvCallbackEvent->buf_, epvCallbackEvent->bufLen_);
            // 有数据可读
            if (n > 0)
            {
                cout << epvCallbackEvent->buf_;
                tempStr.append(epvCallbackEvent->buf_);
            }
            // 对端连接被关闭
            else if (n == 0)
            {
                cout << endl;
                cout << "cfd: " << cfd << " close" << endl;

                epoll_ctl(epvCallbackEvent->epfd_, EPOLL_CTL_DEL, cfd, NULL);
                close(cfd);
                // 释放EpvCallback在堆区的内存
                delete[] epvCallbackEvent->buf_;
                delete epvCallbackEvent;
                break;
            }
            else
            {
                // 没有数据可读了
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    cout << "subthread: " << pthread_self() << "cfd: " << cfd << "EAGAIN | EWOULDBLOCK" << endl;
                    // 将数据直接发送给客户端
                    n = write(cfd, tempStr.data(), tempStr.size());
                    // 成功发送所有数据，不需要修改监听的事件
                    if (n == tempStr.size())
                    {
                        cout << "reader send all data successful..." << endl;
                    }
                    // 只发送了一部分数据，说明内核发送缓冲区满了，暂时发不了了，
                    // 就epoll监听EPOLLOUT事件，如果内核发送缓冲区允许发送了就调用writer发送剩余的数据
                    else if (0 < n < tempStr.size())
                    {
                        cout << "reader send a part of data, remaining data wait writer send..." << endl;
                        epoll_event epv{};
                        epv.events = EPOLLOUT;
                        epv.data.ptr = data;

                        // 这里能不能同时监听读写事件?当前不支持,因为当前读写缓冲区共用一个读写指针readWriterIndex,如果
                        // 内核的发送缓冲区一直是满的,此时剩余待发送的数据存储在读写缓冲区,writer会一直等待被调用,如果此
                        // 时如果客户端又发送数据过来,会触发reader将读写缓冲区的数据全部置空,然后将数据写入到缓冲区,后续
                        // 实现读写双指针就可以同时监听读写事件,即使内核缓冲区满了也不会触发BUG
                        epvCallbackEvent->callback_ = writer;
                        epvCallbackEvent->events_ = EPOLLOUT;

                        int remaining = tempStr.size() - n;
                        // 读写缓冲区大小小于剩余的要发送的数据大小，就重新开辟一个堆区内存用来存放要发送数据
                        if (remaining > epvCallbackEvent->bufLen_)
                        {
                            delete[] epvCallbackEvent->buf_;
                            epvCallbackEvent->buf_ = new char[remaining]();
                            epvCallbackEvent->bufLen_ = remaining;
                        }

                        epvCallbackEvent->readWriteIndex_ = remaining;
                        copy(tempStr.begin() + n, tempStr.end(), epvCallbackEvent->buf_);
                        epoll_ctl(epvCallbackEvent->epfd_, EPOLL_CTL_MOD, cfd, &epv);
                    }
                    // write时发生错误或者对端已经断开
                    else
                    {
                        // n=-1并且errno==EAGAIN或者errno==EWOULDBLOCK,代表一点数据都没发送出去时,
                        // 套接字的内核缓冲区已经满了,什么都不做让epoll_wait继续监听EPOLLOUT事件
                        // 如果不是就关闭套接字释放资源
                        if (errno != EAGAIN && errno != EWOULDBLOCK)
                        {
                            cout << "reader send message error, errno: " << errno << endl;
                            close(cfd);
                            delete[] epvCallbackEvent->buf_;
                            delete epvCallbackEvent;
                        }
                    }
                    break;
                }
                // 被信号打断
                else if (errno == EINTR)
                {
                    cout << "reader EINTR..." << endl;
                }
                // 对端连接被重置（对端通过 TCP RST 包 强制 重置连接）
                else if (errno == ECONNRESET)
                {
                    cout << "reader ECONNRESET..." << endl;
                    epoll_ctl(epvCallbackEvent->epfd_, EPOLL_CTL_DEL, cfd, NULL);
                    close(cfd);
                    // 释放EpvCallback在堆区的内存
                    delete[] epvCallbackEvent->buf_;
                    delete epvCallbackEvent;
                    break;
                }
                // 发生其他系统错误
                else
                {
                    cout << "reader " << "cfd: " << cfd << "error" << " errno:" << errno << endl;
                    epoll_ctl(epvCallbackEvent->epfd_, EPOLL_CTL_DEL, cfd, NULL);
                    close(cfd);
                    // 释放EpvCallback在堆区的内存
                    delete[] epvCallbackEvent->buf_;
                    delete epvCallbackEvent;
                }
            }
        }
        

    }
    
}


void acceptor(int lfd, int events, void* data)
{
    EpvCallback* p = reinterpret_cast<EpvCallback*>(data);
    // 10、调用调用accept从已完成连接队列中取出一个链接，获得与该链接通信的通信套接字
    int cfd = accept4(lfd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (cfd > 0)
    {
        // 11、把cfd通信分配给子线程中的epoll，让子线程去给这个cfd服务
        // 主线程只做一件事情就是接收新连接，将连接分配给子线程
        cout << "mainthread: " << pthread_self() << "hello cfd:" << cfd << endl;
        // 11.1、获得属于子线程epoll的epfd，通过该epfd向子线程的epoll添加通信套接字
        int epfd = p->threadPoll_->getNextSubThtreadEpfd();
        // 11.2、为这个通信套接字创建一个epoll_event
        epoll_event cfdEpv;
        memset(&cfdEpv, 0x00, sizeof(epoll_event));
        
        // 11.3、让subepoll监听读事件
        cfdEpv.events = EPOLLIN;
        // 11.4、为这个epoll_event创建回调
        EpvCallback* epvCallbackEvent = new EpvCallback;
        memset(epvCallbackEvent, 0x00, sizeof(EpvCallback));
        epvCallbackEvent->epfd_ = epfd;
        epvCallbackEvent->fd_ = cfd;
        epvCallbackEvent->events_ = cfdEpv.events;
        epvCallbackEvent->callback_ = reader;
        epvCallbackEvent->threadPoll_ = p->threadPoll_;
        epvCallbackEvent->buf_ = new char[1024];
        epvCallbackEvent->bufLen_ = 1024;
        epvCallbackEvent->readWriteIndex_ = 0;
        // 11.5、将这个epoll_event的回调指针指向epvCallback
        cfdEpv.data.ptr = reinterpret_cast<void*>(epvCallbackEvent);
        // 11.6、将这个epoll_event添加到subepoll上
        epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &cfdEpv);
    }
    else
    {
        // 没有连接请求
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            cout << "EAGAIN || EWOULDBLOCK" << endl;
        }
        else
        {
            cout << "accept4 error, errno:" << errno << endl;
            exit(EXIT_FAILURE);
        }
    }
}


int main() 
{   
    ThreadPoll* threadPoll = threadPollInit(0);

    // 1、创建套接字
    int lfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (lfd < 0)
    {
        exit(EXIT_FAILURE);
    }
    // 1.1、设置ip地址端口复用
    int optval = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // 2、为套接字绑定ip、端口
    // 2.1、创建sockaddr_in并初始化
    sockaddr_in localAddr{};
    // 2.2、通过htons函数确保8888是网络字节序
    localAddr.sin_port = htons(8888);
    // 2.3、确保localAddr.sin_addr.s_addr是网络字节序
    inet_pton(AF_INET, "127.0.0.1", &localAddr.sin_addr.s_addr);
    // 2.4、设置协议
    localAddr.sin_family = AF_INET;
    socklen_t len = sizeof(localAddr);
    // 2.5、调用bind函数进行绑定
    int ret = bind(lfd, (sockaddr*)&localAddr, len);
    if (ret < 0)
    {
        cout << "bind error,errno:" << errno << endl;
    }

    // 3、将套接字设置为监听状态
    ret = listen(lfd, 128);
    if (ret < 0)
    {
        cout << "listen error,errno:" << errno << endl; 
    }

    // 4、创建epoll树根节点
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd < 0)
    {
        cout << "epoll_create1 error,errno:" << errno << endl; 
    }

    threadPoll->mainThreadEpfd = epfd;

    // 5、给监听套接字创建epoll_event
    epoll_event lfdEpv{};
    // 5.1设置epoll_event属性
    lfdEpv.events = EPOLLIN;

    // 5.2准备自定义结构体，自定义结构体中包含了与监听文件描述符绑定的回调函数acceptor
    // 这个变量的生命周期与主线程一样所以没必要在堆区开辟
    EpvCallback epvCallbackEvent{};
    epvCallbackEvent.fd_ = lfd;
    epvCallbackEvent.events_ = lfdEpv.events;
    epvCallbackEvent.callback_ = acceptor;
    epvCallbackEvent.epfd_ = epfd;
    // 设置一下线程池
    epvCallbackEvent.threadPoll_ = threadPoll;

    // 5.3设置epoll_event.data.ptr来指向自定义类型的结构体
    lfdEpv.data.ptr = reinterpret_cast<void*>(&epvCallbackEvent);
    
    //  6、将lfdEpv更新到epoll
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &lfdEpv);
    if (ret < 0)
    {
        cout << "epoll_ctl error,errno:" << errno << endl; 
    }


    // 7、创建一个epoll_event类型的动态数组用来接收epoll监听的返回事件
    // 初始大小是16
    vector<epoll_event> epvs(16);
    // 10000毫秒=10秒
    int epollWaitSecondMs = 10000;

    // 8、通过epoll_wait监听epoll_event事件
    while (true)
    {
        int num = epoll_wait(epfd, &*epvs.begin(), epvs.size(), epollWaitSecondMs);
        // 9、轮询传入给epoll_wait的epoll_event数组
        if (num > 0)
        {
            auto it = epvs.begin();
            for (int i = 0; i < num; i++, it++)
            {
                EpvCallback* p = reinterpret_cast<EpvCallback*>(it->data.ptr);
                p->callback_(p->fd_, it->events, it->data.ptr);
                // if (it->events & EPOLLIN && p->fd_ == lfd)
                // {
                //     // 9.1 调用acceptor（新连接的处理只在主线程中进行）
                //     p->callback_(p->fd_, it->events, it->data.ptr);
                // }
                // test
                
            }
        }
        else if (num == 0)
        {
            cout << "mainthread: " << pthread_self() << " epoll_wait timeout..." << endl;
        }
        else
        {
            // epoll_wait被信号打断
            if (errno != EINTR)
            {
                cout << "epoll_wait error, errno:" << errno << endl;
                exit(EXIT_FAILURE);
            }
        } 
    }

    // 销毁线程池
    threadPollDestory(threadPoll);
    // 关闭监听文件描述符
    close(lfd);
    // 关闭主线程的epoll
    close(epfd);
    return 0;
}