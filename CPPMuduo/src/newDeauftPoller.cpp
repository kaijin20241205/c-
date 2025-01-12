#include "./../include/Poller.hpp"
#include "./../include/Epoller.hpp"

#include <stdlib.h>


Poller* Poller::newDeauftPoller(EventLoop* eventLoop)
{
    if (getenv("MUDUO_USE_POLL"))
    {
        // 还未兼容POLL
        return nullptr;
    }
    else if (getenv("MUDUO_USE_SELECT"))
    {
        // 还未兼容SELECT
        return nullptr;
    }
    // 默认使用epoll
    else
    {
        return new Epoller(eventLoop);
    }
}