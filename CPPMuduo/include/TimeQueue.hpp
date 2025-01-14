// #pragma once
// #include "./noncopyable.hpp"
// // #include "./../include/Channel.hpp"

// #include <memory>           // std::pair、std::shared_ptr
// #include <set>
// #include <vector>

// 定时器管理类
// class Timer;
// class Channel;
// class Timestamp;
// class EventLoop;
// class TimeQueue : noncopyable
// {
// public:
//     // 构造
//     TimeQueue(EventLoop* eventLoop);
//     // 析构
//     ~TimeQueue();
//     // 给定时任务集合添加新的定时器
//     void addTimer(Timer* timer);
//     // 取消定时器
//     void cancle(Timer* timer);
// private:
//     using Entry = std::pair<Timestamp, std::shared_ptr<Timer>>;
//     using TimerList = std::vector<Entry>;
//     // 定时器对象被唤醒时的处理函数
//     void handleRead();
//     // 获得所有过期的定时器
//     TimerList getExpireTimers(Timestamp now);
//     // 给已经添加的定时器重置过期时间
//     void resetTimer(Timer* tiemr);


    
//     // 某个事件循环实例化对象（一般是主线程的EventLoop）
//     EventLoop* eventLoop_;
//     // timerfd（linux下的一个定时器对象）
//     int timeFd_;
//     // timerfd的Channel实例化对象
//     Channel timeFdChannel_;
//     // 定时任务集合<timestamp,std::share_ptr>（按照过期日期降序排序）
//     std::set<Entry> timers_;
// };