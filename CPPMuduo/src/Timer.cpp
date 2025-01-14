#include "./../include/Timer.hpp"


// 初始化类的静态成员
std::atomic<int64_t> Timer::createSenquence_;

// 给重复类型的定时器重新计算定时器的下一次到期时间）
void Timer::restart(Timestamp now)
{
    if (repeat_)
    {
        expiration_ = addTime(now, period_);
    }
    // 不是重复类型的定时器设置一个无效的时间戳
    else
    {
        expiration_ = Timestamp::invaild();
    }
}