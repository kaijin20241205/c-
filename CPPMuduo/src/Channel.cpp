#include "./../include/Channel.hpp"
#include "./../include/Timestamp.hpp"
#include "./../include/EventLoop.hpp"

#include <sys/epoll.h>

// 空事件
const int Channel::kNoneEvent = 0;
/*
EPOLLIN (0x001): 表示文件描述符上有数据可读。例如，TCP 套接字接收到数据、管道中有数据可读等。
EPOLLPRI (0x002): 表示文件描述符上有优先级数据可读。优先级数据通常是带外数据（Out-Of-Band Data），
在 TCP 套接字中这种数据通常是通过 send 或 sendmsg 函数配合 MSG_OOB 标志发送的。
*/ 
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
/*
EPOLLOUT (0x004): 当文件描述符的发送缓冲区有空闲空间，可以安全地写入数据时，EPOLLOUT 事件触发。
EPOLLHUP (0x010): 当连接断开或文件描述符挂起时触发，常见于以下情况：
                    对端关闭连接（主动关闭或异常退出）。
                    管道的一端关闭。
                    文件描述符不再可用。
*/
const int Channel::kWriteEvent = EPOLLOUT | EPOLLHUP;

/*
EPOLLERR事件会被自动监听无需显示的指定，EPOLLERR 是一种错误事件，用于表示文件描述符出现了某种错误。
常见触发场景包括：
套接字发生错误，例如网络连接中断。
写操作时对端已关闭。
管道破裂。
文件描述符不可用或无效。
一旦发生这些错误，内核会将该事件通知给 epoll，即便你没有显式关注 EPOLLERR。
*/

// 更新channel的状态
void Channel::update()
{
    eventLoop_->updateChannel(this);
}

// 回调函数的防卫函数，来确保channel对象在生命周期内调用回调函数
void Channel::handleEventWithGuard(Timestamp timestamp)
{
    // 如果tied_存在说明channel的生命周期依赖于其他的对象，调用回调函数时需要检查channel是否已经被释放了
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handleEvent(timestamp);
        }
    }
    // tied_不存在，说明channel是独立存在的，不依赖外部对象，生命周期由自己管理，无需检查直接调用回调
    else
    {
        handleEvent(timestamp);   
    }
}

// 执行回调
void Channel::handleEvent(Timestamp timestamp)
{
    // 如果远端关闭了连接（例如close）并且缓冲区已经没有远端的数据要读取了，就关闭该连接
    if ((revent_ & EPOLLHUP) && !(revent_ & EPOLLIN))
    {
        if (closeEventCallback_)
        {
            closeEventCallback_();
        }
    }
    // 缓冲区有远端发来的数据，去读取
    if ((revent_ & EPOLLIN) || (revent_ & EPOLLPRI))
    {
        if (readEventCallback_)
        {
            readEventCallback_(timestamp);
        }
    }
    // 缓冲区有空间可以写入数据，去写入
    if (revent_ & EPOLLOUT)
    {
        if (writeEventCallback_)
        {
            writeEventCallback_();
        }
    }
    // 有错误事件发生，去处理（当前的实现并没有设置监听错误处理的函数，只有读、写相关）
    if (revent_ & EPOLLERR)
    {
        if (errorEventCallback_)
        {
            errorEventCallback_();
        }
    }
}

// 设置弱智能指针
void Channel::setTie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}

// 在POLLER上删除channel
void Channel::remove()
{
    eventLoop_->removeChannel(this);
}