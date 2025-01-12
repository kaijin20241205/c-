#include "./../include/Poller.hpp"
#include "./../include/Channel.hpp"


bool Poller::hasChannel(Channel* channel) const
{
    int fd = channel->getFd();
    auto it = channelMap_.find(fd);
    if (it != channelMap_.end())
    {
        return true;
    }
    return false;
}