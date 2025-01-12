#include "./../include/Socket.hpp"
#include "./../include/InetAddress.hpp"
#include "./../include/Logger.hpp"


#include <unistd.h>         // close
#include <strings.h>        // bzero
#include <netinet/tcp.h>    // TCP_NODELAY


Socket::~Socket()
{
    close(fd_);
}

// 给套接字绑定ip和port
void Socket::bindIpPort(const InetAddress& addr)
{
    socklen_t len = sizeof(sockaddr_in);
    if (0 != bind(fd_, (sockaddr*)addr.getSockaddrIn(), len))
    {
        LOG_FATAL("Socket::bindIpPort fail, fd=[%d]", fd_);
    }
}
// 给套接字绑设置监听状态
void Socket::listenFd()
{
    if (0 != listen(fd_, 1024))
    {
        LOG_FATAL("Socket::listenFd fail, fd=[%d]", fd_);
    }
}
// 接受客户端连接请求
int Socket::acceptFd(InetAddress& perrAddr)
{
    sockaddr_in addr;
    socklen_t len = sizeof(sockaddr_in);
    bzero(&addr, sizeof(sockaddr_in));
    int connFd = accept4(fd_, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connFd >= 0)
    {
        perrAddr.setAddrIn(addr);
    }
    return connFd;
}

// 关闭写端
void Socket::shutdownWrite(bool on)
{
    if (shutdown(fd_, SHUT_WR) < 0)
    {
        LOG_ERROR("Socket::shutdownWrite fail");
    }
}

// 设置端口复用
void Socket::setPortReuse(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

// 设置sockaddr地址复用
void Socket::setAddrReuse(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

// 设置连接心跳检测
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}

// 设置nagle算法
void Socket::setNodelay(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}