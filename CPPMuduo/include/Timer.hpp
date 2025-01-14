#pragma once
#include "./noncopyable.hpp"
#include "./../include/Timestamp.hpp"

#include <functional>
#include <atomic>


// 定时器类
class Timer : noncopyable
{
public:
    using TimerCallback = std::function<void()>;
    // 有参构造
    Timer(const TimerCallback cb, Timestamp when, double period) : 
    timerCallback_(std::move(cb)), 
    expiration_(when),
    repeat_(period_ > 0.0),
    period_(period), 
    senquence_(incrementAndGet()) {};

    // 运行定时器的回调函数
    void run() const { if (timerCallback_) {timerCallback_();} };
    // 返回定时器的到期时间
    Timestamp getTimerExpiration() const { return expiration_; };
    // 返回定时是否属于重复定时器
    bool isRepeat() const { return repeat_; };
    // 给重复类型的定时器重新计算定时器的下一次到期时间）
    void restart(Timestamp now);
    /* 
    每实例化一个对象就对唯一标识+1，senquence_从1开始
    memory_order_relaxed：通常用于 计数器、统计器、日志标记 等场景，线程不需要对这些操作的先后顺序有严格的保证。
    */
    int incrementAndGet() { return (createSenquence_.fetch_add(1, std::memory_order_relaxed) + 1); };

private:
    // 定时器回调函数
    const TimerCallback timerCallback_;
    // 定时器到期时间
    Timestamp expiration_;
    // 是否属于重复定时器
    const bool repeat_;
    // 定时器的重复周期
    double period_;
    /* 
    唯一标识：没太明白这个表示到底有什么作用，我们已经使用了Timer的指针地址作为二叉树的唯一key，所以
    当前好像没有这个的应用场景。
    */
    int64_t senquence_;
    // 用来对senquence_进行原子性的递增
    static std::atomic<int64_t> createSenquence_;
};