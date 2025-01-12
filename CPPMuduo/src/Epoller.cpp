#include "./../include/Epoller.hpp"
#include "./../include/Logger.hpp"
#include "./../include/Timestamp.hpp"
#include "./../include/Channel.hpp"

#include <unistd.h>         // close


// 未添加到epoll标志
const int kNew = -1;
// 已添加到epoll标志
const int kAdded = 1;
// 已从epoll中删除
const int kDeleted = 2;

Epoller::Epoller(EventLoop* eventLoop) : 
    Poller(eventLoop), 
    epfd_(::epoll_create1(EPOLL_CLOEXEC)), 
    eventList_(kInitEventListSize)
{
    if (epfd_ < 0)
    {
        LOG_FATAL("Epoller::Epoller(EventLoop* eventLoop), epoll_create1 error, errno:%d", errno);
    }
}

Epoller::~Epoller()
{
    ::close(epfd_);
}

// 重写基类的抽象方法1，更新channel方法（提供给外部调用的：EventLoop）
void Epoller::updateChannel(Channel* channel)
{
    int index = channel->getIndex();
    // 是已删除、未添加的
    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            int fd = channel->getFd();
            channelMap_[fd] = channel;
        }
        channel->setIndex(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    // 是已添加的
    else
    {
        // 如果对任何事件都不感兴趣，就从epoll上删除它
        if (channel->isNonEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->setIndex(kDeleted);
        }
        // 对事件感兴趣说明可能修改过监听事件，在epoll上更新它
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 重写基类的抽象方法2，删除channle方法（提供给外部调用的：EventLoop）
void Epoller::removeChannel(Channel* channel)
{
    // 从Map中删除该channel
    int fd = channel->getFd();
    channelMap_.erase(fd);
    // 从epoll上删除该channl对应的套接字
    if (channel->getIndex() == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->setIndex(kNew);
}

// 重写基类的抽象方法3，io多路复用机制的启动函数，返回值是时间戳（提供给外部调用的：EventLoop）
Timestamp Epoller::loop(const int timeoutMs, ChannelList* activeChannels)
{   
    int eventNum = ::epoll_wait(epfd_, &*eventList_.begin(), static_cast<int>(eventList_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    // 有事件发生
    if (eventNum > 0)
    {
        LOG_DEBUG("Timestamp Epoller::loop(const int timeoutMs, ChannelList* activeChannels) epoll_wait happend...");
        fillActiveChannels(eventNum, activeChannels);
        if (eventNum == eventList_.size())
        {
            eventList_.resize(eventList_.size() * 2);
        }
    }
    // timeoutMs时间段内没有事件发生
    else if (eventNum == 0)
    {
        LOG_DEBUG("Timestamp Epoller::loop(const int timeoutMs, ChannelList* activeChannels) epoll_wait timeout...");
    }
    // epoll_wait发生错误
    else
    {
        // 不是被信号打断的错误
        if (saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("Timestamp Epoller::loop(const int timeoutMs, ChannelList* activeChannels). epoll_wait error, errno:%d", errno);
        }
    }

    return now;
}

// 更新channel
void Epoller::update(int operation, Channel* channel)
{
    epoll_event evp = {};
    evp.events = channel->getEvents();
    evp.data.ptr = channel;
    if (epoll_ctl(epfd_, operation, channel->getFd(), &evp) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("void Epoller::update(int operation, Channel* channel) epoll_ctl del error, errno:%d", errno);
        }
        else
        {
            LOG_FATAL("void Epoller::update(int operation, Channel* channel) epoll_ctl add/mod error, errno:%d", errno);
        }
    }
}

// 将epoll返回的活跃事件写入传入参数
void Epoller::fillActiveChannels(int evnetNum, ChannelList* channelList) const
{
    for (int i = 0; i < evnetNum; i++)
    {
        Channel* channel = static_cast<Channel*>(eventList_[i].data.ptr);
        channel->setRevents(eventList_[i].events);
        channelList->push_back(channel);
    }
}