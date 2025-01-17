#pragma once
#include "./noncopyable.hpp"
#include "./Channel.hpp"
#include "./Callbacks.hpp"

#include <memory>           // std::pair、std::shared_ptr
#include <set>
#include <vector>

// 定时器管理类
class Timer;
class Timestamp;
class EventLoop;
class TimeQueue : noncopyable
{
public:
    // 构造
    TimeQueue(EventLoop* eventLoop);
    // 析构
    ~TimeQueue();
    // 给定时任务集合添加新的定时器
    void addTimer(const TimerCallback cb, Timestamp when, double period);
    // 取消定时器
    // void cancle(Timer* timer);
private:
    using Entry = std::pair<Timestamp, std::shared_ptr<Timer>>;
    /* 
    （std::set/std::map）内部是二叉搜索树，以pair<Timestamp, std::shared_ptr<Timer>>为key，
    这样即便两个Timer的到期时间相同，它们的地址也必定不同，实现了key的唯一性，需要实现Timestamp的opearotr<
    */
    using TimerList = std::set<Entry>;
    // 定时任务集合<timestamp,std::share_ptr>（按照过期日期升序排序）
    std::set<Entry> timers_;
    // 某个事件循环实例化对象（一般是主线程的EventLoop）
    EventLoop* eventLoop_;
    // timerfd（linux下的一个定时器对象）
    const int timeFd_;
    // timerfd的Channel实例化对象
    Channel timeFdChannel_;

    // 定时器对象被唤醒时的处理函数
    void handleRead();
    // 获得所有过期的定时器
    std::vector<Entry> getExpireTimers(Timestamp now);
    // 给已经添加的定时器重置过期时间
    void resetTimers(const std::vector<Entry>& expireds, Timestamp now);
    // 将定时器插入到定时器集合中--提供给addTimerInLoop的添加接口
    bool insert(Timer* timer);
    // 在自己的EventLoop中添加Timer，提供给addTimer的添加接口
    void addTimerInLoop(Timer* timer);
};