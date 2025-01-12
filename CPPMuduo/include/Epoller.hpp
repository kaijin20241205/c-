#pragma once
#include "./Poller.hpp"
#include "./noncopyable.hpp"

#include <vector>
#include <sys/epoll.h>


class Epoller : public Poller
{
public:
    Epoller(EventLoop* eventLoop);
    ~Epoller();
    // 重写基类的抽象方法1，更新channel方法（提供给外部调用的：EventLoop）
    void updateChannel(Channel* channel) override;
    // 重写基类的抽象方法2，删除channle方法（提供给外部调用的：EventLoop）
    void removeChannel(Channel* channel) override;
    // 重写基类的抽象方法3，io多路复用机制的启动函数，返回值是时间戳（提供给外部调用的：EventLoop）
    Timestamp loop(const int timeoutMs, ChannelList* activeChannels) override;
private:
    using EventList = std::vector<epoll_event>;
    // epoll返回的监听事件数组大小的初始值
    static const int kInitEventListSize = 16;
    // epoll的句柄
    int epfd_;
    // epoll返回的监听事件数组
    EventList eventList_;
    // 更新channel
    void update(int operation, Channel* channel);
    // 将epoll返回的活跃事件写入传入参数
    void fillActiveChannels(int evnetNum, ChannelList* channelList) const;
};