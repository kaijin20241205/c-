#include "./../include/TimeQueue.hpp"
#include "./../include/Channel.hpp"
#include "./../include/Logger.hpp"
#include "./../include/Timestamp.hpp"
#include "./../include/Timer.hpp"

#include <sys/timerfd.h>
#include <unistd.h>             // close
#include <iterator>             // back_insert

// 创建定时器
int createTimerFd()
{
    int timerFd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerFd < 0)
    {
        LOG_FATAL("int createTimerFd() timer_create error, errno:%d", errno);
    }
    return timerFd;
}

// 构造函数
TimeQueue::TimeQueue(EventLoop* eventLoop) : 
    eventLoop_(eventLoop), 
    timeFd_(createTimerFd()), 
    timeFdChannel_(timeFd_, eventLoop_),
    timers_()
    {
        // 设置定时器的回调函数
        timeFdChannel_.setReadEventCallback(
            std::bind(&TimeQueue::handleRead, this));
        // 监听读事件
        timeFdChannel_.setEnableReading();
    }

// 析构函数
TimeQueue::~TimeQueue()
{
    // 不对任何事件感兴趣
    timeFdChannel_.setDisableAll();
    // 将定时器channel从POLLER上删除
    timeFdChannel_.remove();
    // 关闭定时器timefd
    ::close(timeFd_);
    // 清空定时器timer合集
    timers_.clear();
}

// 给定时器集合添加定时器
void addTimer(const TimerCallback cb, Timestamp when, double period)
{
    // 创建一个Timer
    Timer* timer = new Timer(std::move(cb), when, period);
    // 将timer添加所属的
}

// 定时器对象被唤醒时的处理函数
void TimeQueue::handleRead()
{

}

// 获得所有过期的定时器
std::vector<Entry> TimeQueue::getExpireTimers(Timestamp now)
{
    /*
    整体流程
    1、找到所有触发时间早于 now 的定时器，放入到 expired 集合中。
    2、从定时器集合中移除这些超时定时器。
    3、对于重复定时器，重新计算触发时间并插入集合-->这一步不在本函数内实现，由定时任务的回调函数实现
    本函数只完成一个功能就是从定时器集合中分离出时间戳小于now的所有定时器并将这些定时器返回
    4、返回超时定时器集合。
    */
    // 过期定时器集合
    TimerList expired;
    // 哨兵值
    Entry sentry = std::make_pair(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    // 在定时器（有序）合集中寻找第一个大于哨兵值的位置
    auto it_end = timers_.lower_bound(sentry);
    // 将过期的定时器都拷贝到过期定时器集合中
    /*
    back_inserter 是 C++ 标准库中提供的一个辅助工具，位于 <iterator> 头文件中。它用于为一个支持尾部插入操作的容器
    （如 std::vector、std::deque 或 std::list）创建一个 迭代器，从而将新元素插入到容器的末尾。
    简而言之，std::back_inserter 能将元素自动追加到容器的末尾，而无需显式调用容器的 push_back 方法。
    */
    std::copy(timers_.begin(), it_end, std::back_inserter(expired));
    // 将定时器集合中的到期定时器删除掉
    timers_.erase(timers_.begin(), it_end);
    // 返回到期的定时器集合
    return expired;
}

// 给已经添加的定时器重置过期时间
// void TimeQueue::resetTimer(Timer* tiemr)
// {

// }

// bool TimeQueue::insert(Timer* timer)
// {

// }
