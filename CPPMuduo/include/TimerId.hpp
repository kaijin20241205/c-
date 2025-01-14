#pragma once
#include "./copyable.hpp"

#include <memory>       // std::share_ptr


// 定时器标识类
class Timer;
class TimerId : copyable
{
public:
    TimerId(std::shared_ptr<Timer> timer, int64_t senquence) :
        timer_(timer), 
        senquence_(senquence)
        {};
private:
    // 指向Timer的弱智能指针
    std::weak_ptr<Timer> timer_;
    // Timer的唯一标识（当前没有这个数据的应用场景）
    int64_t senquence_;
};

