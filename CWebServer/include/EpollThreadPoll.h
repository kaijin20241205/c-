#ifndef EPOLL_THREAD_POLL
#define EPOLL_THREAD_POLL
#include <pthread.h>
#include <stdint.h>         // uint64_t


// 线程池结构体
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
    int mainThreadEpfd_;

    // 主、子线程用来通信的文件描述符，来控制按照次序创建子线程
    int evnetFd_;
};

// 轮询subThtreadEpfds_数组获得一个epfd
int getNextSubThtreadEpfd(EpollThreadPoll* epollThreadPoll);

// 初始化线程池
EpollThreadPoll* epollThreadPollInit(int threadNum);

// 销毁线程池
void epollThreadPollDestory(EpollThreadPoll** epollThreadPoll);

// 子线程工作函数
void threadWorkStart(EpollThreadPoll* epollThreadPoll, int epfdIndex);

// 子线程初始化
void* threadWorkInit(void* data);

// 启动线程池-开始创建子线程
void epollThreadPollStart(EpollThreadPoll* epollThreadPoll);

#endif