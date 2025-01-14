// #include "./../include/TimeQueue.hpp"
// #include "./../include/Channel.hpp"
// #include "./../include/Logger.hpp"

// #include <sys/timerfd.h>
// #include <unistd.h>             // close


// // 创建定时器
// int createTimerFd()
// {
//     int timerFd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
//     if (timerFd < 0)
//     {
//         LOG_FATAL("int createTimerFd() timer_create error, errno:%d", errno);
//     }
//     return timerFd;
// }



// TimeQueue::TimeQueue(EventLoop* eventLoop) : 
//     eventLoop_(eventLoop), 
//     timeFd_(createTimerFd()), 
//     timeFdChannel_(timeFd_, eventLoop_),
//     timers_()
//     {
//         // 设置定时器的回调函数
//         timeFdChannel_.setReadEventCallback(
//             std::bind(&TimeQueue::handleRead, this));
//         // 监听读事件
//         timeFdChannel_.setEnableReading();
//     }

// TimeQueue::~TimeQueue()
// {
//     // 不对任何事件感兴趣
//     timeFdChannel_.setDisableAll();
//     // 将channel从POLLER上删除
//     timeFdChannel_.remove();
//     // 关闭timefd
//     ::close(timeFd_);
//     // 清空timer合集
//     timers_.clear();
// }


// void TimeQueue::addTimer(Timer* timer)
// {

// }

