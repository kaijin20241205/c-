#include "./../include/EventLoop.hpp"
#include "./../include/Poller.hpp"
#include "./../include/Logger.hpp"
#include "./../include/Channel.hpp"


// 定义一个全局但每个线程都唯一互补干扰的变量，通过该变量来防卫一个线程创建一个以上的EventLoop实例化对象
thread_local EventLoop* t_eventLoopThisThread = nullptr;

// 定义默认的IO多路复用超时时间
const int kPollerTimeoutMS = 10000;

EventLoop::EventLoop() : 
    threadId_(CurrentThread::tid()), 
    poller_(Poller::newDeauftPoller(this)), 
    looping_(false), 
    quit_(false)
{
    // 一个线程创建了2个及以上的EventLoop实例化对象，触发FATAL日志打印
    if (t_eventLoopThisThread)
    {
        LOG_FATAL("Anoter Event %p is exists in this thread %d", t_eventLoopThisThread, threadId_);
    }
    else
    {
        t_eventLoopThisThread = this;
    }
}

EventLoop::~EventLoop()
{
    looping_ = false;
    t_eventLoopThisThread = nullptr;
}

// 要点1：基本功能
// 开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;
    
    while (!quit_)
    {
        activeChannels_.clear();
        pollerReturnTime_ = poller_->loop(kPollerTimeoutMS, &activeChannels_);
        for (Channel* channel: activeChannels_)
        {
            channel->handleEvent(pollerReturnTime_);
        }
    }
    looping_ = false;
}

// 退出事件循环
void EventLoop::quit()
{
    quit_ = true;
}

// IO多路复用机制上更新某个channel
void EventLoop::updateChannel(Channel* channel)
{
    poller_->updateChannel(channel);
}

// IO多路复用机制上删除某个Channel
void EventLoop::removeChannel(Channel* channel)
{
    poller_->removeChannel(channel);
}

// IO多路复用机制上是否存在某个Channel
bool EventLoop::hasChannel(Channel* channel)
{
    return poller_->hasChannel(channel);
}