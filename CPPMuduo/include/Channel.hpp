#pragma once
#include "./noncopyable.hpp"

#include <functional>       // function
#include <memory>           // weak_ptr


class Timestamp;
class EventLoop;
class Channel : noncopyable
{
public:
    using ReadEventCallback = std::function<void(Timestamp)>;
    using EventCallback = std::function<void()>;
    // 有参构造
    Channel(int fd, EventLoop* eventLoop) : 
        fd_(fd), 
        eventLoop_(eventLoop), 
        event_(0), 
        revent_(0), 
        index_(-1),
        tied_(false)
        {};
    // 析构
    ~Channel() = default;
    // 执行回调
    void handleEvent(Timestamp timestamp);
    // 设置回调（读、写、关闭、错误）
    void setReadEventCallback(ReadEventCallback cb) { readEventCallback_ = std::move(cb); };
    void setWriteEventCallback(EventCallback cb) { writeEventCallback_ = std::move(cb); };
    void setErrorEventCallback(EventCallback cb) { errorEventCallback_ = std::move(cb); };
    void setCloseEventCallback(EventCallback cb) { closeEventCallback_ = std::move(cb); };
    // 设置弱智能指针
    void setTie(const std::shared_ptr<void>&);
    // 返回文件描述符
    int getFd() const { return fd_; };
    // 返回当前监听的文件描述符
    int getEvents() const { return event_; };
    // 返回POLLER返回的监听的事件
    int getRevents() const { return revent_; };
    // 设置POLLER返回的监听事件
    void setRevents(int revents) { revent_ = revents; };
    // 设置监听事件并更新到POLLER（可读、不可读、可写、不可写、全部禁止）
    void setEnableReading() { event_ |= kReadEvent; update(); };
    void setDisableReading() { event_ &= ~kReadEvent; update(); };
    void setEnableWriting() { event_ |= kWriteEvent; update(); };
    void setDisableWriting() { event_ &= ~kWriteEvent; update(); };
    void setDisableAll() { event_ = kNoneEvent; update(); };
    // 返回当前监听的事件状态（读、写、空）
    bool isReading() const { return event_ & kReadEvent; };
    bool isWriting() const { return event_ & kWriteEvent; };
    bool isNonEvent() const { return event_ == kNoneEvent; };
    // 返回当前channel在POLLER上的状态
    int getIndex() const { return index_; };
    // 设置channel在POLLER上的状态
    void setIndex(int index) { index_ = index; };
    // 返回channel所属的EVENTLOOP
    EventLoop* ownerEventLoop () const { return eventLoop_; };
    // 在POLLER上删除channel
    void remove();
private:
    /*
    轻度抽象的事件标志，在IO多路复用机制的事件与IO多路复用机制中加了一层中间层，但是又没有彻底的抽象，
    如果要将兼容多种IO多路复用机制这个事件标志还是要重写的，设置称类的静态私有属性，只允许类内访问
    */
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;
    // 文件描述符fd
    int fd_;
    // 事件循环对象EventLoop
    EventLoop* eventLoop_;
    // 需要监听的事件
    int event_;
    // POLLER返回的事件
    int revent_;
    // 监听的事件发生时需要执行的回调函数
    ReadEventCallback readEventCallback_;
    EventCallback writeEventCallback_;
    EventCallback closeEventCallback_;
    EventCallback errorEventCallback_;
    /*
    channel在IO多路复用机制的标志位，用来表示channel是否已经添加到IO多路复用机制中？或者说未添加到IO多路复用机制中但
    是已经存在于Poller的派生类的channelMap中了（这种情况属于被IO多路复用机制删除但是还没从channelMap中删除）
    */
    int index_;
    // 弱智能指针，用来表示channel是否已经被销毁
    std::weak_ptr<void> tie_;
    /*
    标识位，用来表示弱智能指针是否存在提高性能，毕竟当弱智能指针重来没有被赋过值的情况下，检查一个bool是
    否存在比调用弱智能指针的.lock()方法再去检查lock方法返回的对象是否存在要快
    */
    bool tied_;
    // 更新channel的状态
    void update();
    // 回调函数的防卫函数，来确保channel对象在生命周期内调用回调函数
    void handleEventWithGuard(Timestamp timestamp);
};