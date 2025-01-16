#pragma once
#include "./noncopyable.hpp"
#include "./../include/CurrentThreadId.hpp"
#include "./../include/Timestamp.hpp"

#include <sys/types.h>      // pid_t
#include <memory>           // std::unique_ptr
#include <vector>
#include <atomic>


class Poller;
class Channel;
class EventLoop : noncopyable
{
public:
    // 构造函数：
    EventLoop();
    ~EventLoop();
    // 要点1：基本功能
    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();
    // IO多路复用机制上更新某个channel
    void updateChannel(Channel*);
    // IO多路复用机制上删除某个Channel
    void removeChannel(Channel*);
    // IO多路复用机制上是否存在某个Channel
    bool hasChannel(Channel*);
    // 检查当前运行的EventLoop实例是否运行在创建该EventLoop的线程上
    bool isInLoopInThread() const { return threadId_ == CurrentThread::tid(); };
private:
    using ChannelList = std::vector<Channel*>;
    // 要点1：基本功能
    // 当前运行的线程id
    pid_t threadId_;
    // IO多路复用对象
    std::unique_ptr<Poller> poller_;
    // IO多路复用对象返回的活跃的Channel对象
    ChannelList activeChannels_;
    // IO多路复用对象返回的活跃的Channel对象时的时间戳
    Timestamp pollerReturnTime_;
    // 是否正在运行事件循环
    std::atomic<bool> looping_;
    // 是否退出事件循环
    std::atomic<bool> quit_;



    // // eventfd,子线程通知主线程的文件描述符
    // int eventFd_;
    // // eventfd的channel对象
    // Channel evnetFdChannel_;
};