#include "EpollThreadPoll.h"
#include "MyError.h"
#include "EpollEventCallback.h"

#include <stdlib.h>             // malloc free
#include <sys/epoll.h>
#include <string.h>             // memset
#include <sys/eventfd.h>
#include <unistd.h>             // read
#include <errno.h>
#include <stdio.h>


int getNextSubThtreadEpfd(EpollThreadPoll* epollThreadPoll)
{
    if (epollThreadPoll->threadNum_ < 1)
    {
        return epollThreadPoll->mainThreadEpfd;
    }
    // 通过取余操作来不停轮询
    return epollThreadPoll->subThtreadEpfds_[epollThreadPoll->subThreadEpfdsIndex_++ % epollThreadPoll->threadNum_];
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
    EpollThreadPoll* epollThreadPoll = (EpollThreadPoll*)malloc(sizeof(EpollThreadPoll));
    errorExit(
        epollThreadPoll == NULL,
        "epollThreadPollInit malloc EpollThreadPoll error",
        __FILE__, 
        __LINE__
    );
    memset(epollThreadPoll, 0x00, sizeof(EpollThreadPoll));

    // 创建主线程的epoll
    epollThreadPoll->mainThreadEpfd = epollCreate();

    // 2.1 如果参数threadNum小于1，相当于只有一个线程在工作，不需要对线程池的其他属性初始化了，直接返回
    if (threadNum < 1)
    {
        return epollThreadPoll;
    }

    // 启动的函数不在这里面，初始化只负责初始化，不用管其他的，其他的全部搬到start里面去
    // 2.2、threadNum >= 1，对线程池属性进行初始化操作
    // 设置线程数量
    epollThreadPoll->threadNum_ = threadNum;
    // 线程池是否要摧毁
    epollThreadPoll->destoryFlag = false;
    // 线程id数组
    epollThreadPoll->threads_ = (pthread_t*)malloc(sizeof(pthread_t) * threadNum);
    errorExit(
        epollThreadPoll->threads_ == NULL,
        "epollThreadPollInit malloc pthread_t error", 
        __FILE__, 
        __LINE__
    );
    // 初始化线程id数组
    memset(epollThreadPoll->threads_, 0x00, sizeof(pthread_t) * threadNum);
    // 存储所有子线程的epfd的数组
    epollThreadPoll->subThtreadEpfds_ = (int*)malloc(sizeof(int) * threadNum);
    errorExit(
        epollThreadPoll->subThtreadEpfds_ == NULL,
        "epollThreadPollInit malloc int[threadNum] error", 
        __FILE__, 
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
        "epollThreadPollInit eventfd error", 
        __FILE__, 
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
                                (void*)epollThreadPoll);
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

void epollThreadPollDestory(EpollThreadPoll** epollThreadPoll)
{
    if (epollThreadPoll != NULL && *epollThreadPoll != NULL)
    {
        (*epollThreadPoll)->destoryFlag = true;
        if ((*epollThreadPoll)->threads_ != NULL && (*epollThreadPoll)->threadNum_ >= 1)
        {
            for (int i = 0; i < (*epollThreadPoll)->threadNum_; i++)
            {
                if ((*epollThreadPoll)->threads_[i] != 0)
                {
                    pthread_join((*epollThreadPoll)->threads_[i], NULL);
                }
            }
            free((*epollThreadPoll)->threads_);
            (*epollThreadPoll)->threads_ = NULL;
        }

        if ((*epollThreadPoll)->subThtreadEpfds_ != NULL)
        {
            free((*epollThreadPoll)->subThtreadEpfds_);
            (*epollThreadPoll)->subThtreadEpfds_ = NULL;
        }
        // 关闭主线程epfd
        close((*epollThreadPoll)->mainThreadEpfd);
        free(*epollThreadPoll);
        *epollThreadPoll = NULL;
    }
}

// 子线程运行函数
void* threadWorkFunc(void* data)
{
    // 1、获取线程池对象
    EpollThreadPoll* epollThreadPoll = (EpollThreadPoll*)data;
    printf("No:%d, subthread: %lu, create begin...\n", 
            epollThreadPoll->subThreadEpfdsIndex_ + 1, 
            pthread_self()
            );
    // 2、创建子线程epoll
    int subEpfd = epoll_create1(EPOLL_CLOEXEC);
    // 3、准备好写入eventfd的数据
    // 数据值大于0表示事件状态设置为已发生，如果成功写入主线程的read函数会解除阻塞
    uint64_t message = 1;
    if (subEpfd < 0)
    {
        // 3.1、如果创建epfd失败,设置线程分离,通知主线程解除阻塞,创建下一个线程
        printf("subthread: %lu, epoll_create1 error\n", pthread_self());
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