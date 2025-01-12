#pragma once
#include "./noncopyable.hpp"

#include <unordered_map>
#include <vector>

class Channel;
class EventLoop;
class Timestamp;
// 是一个IO多路复用机制（EPOLL、SELECT、POLL）的抽象类，不允许拷贝、赋值
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;
    Poller(EventLoop* eventLoop) : ownerLoop_(eventLoop) {};
    virtual ~Poller() = default;
    // 更新channel方法、添加、删除、修改
    virtual void updateChannel(Channel* channel) = 0;
    // 删除channle方法，在IO多路复用机制对象上删除channel并且在channel合集中删除对应的channel）
    virtual void removeChannel(Channel* channel) = 0;
    // io多路复用机制的启动函数，返回值是时间戳
    virtual Timestamp loop(const int timeoutMs, ChannelList* activeChannels) = 0;
    // 简单静态工厂方法
    static Poller* newDeauftPoller(EventLoop* eventLoop);
    // 检查channel是否在channelMap中（需要等到Channel类被实现后才能实现）
    bool hasChannel(Channel* channel) const;
protected:
    using ChannelMap = std::unordered_map<int, Channel*>;
    // 拥有所有添加到IO多路复用机制上的channel合集（channelMap）
    ChannelMap channelMap_;
private:
    /*
    私有化事件循环对象（EVENTLOOP），因为Poller及其派生类是EVENTLOOP的一个组件，
    从设计角度上考虑，Poller及其派生类不应该允许访问EVENTLOOP，
    而EVENTLOOP可以访问POLLER及其派生类
    */ 
    EventLoop* ownerLoop_;
};