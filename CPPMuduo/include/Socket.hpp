#pragma once
#include "./noncopyable.hpp"

class InetAddress;
class Socket : noncopyable
{
public:
    // 禁止隐式类型转换-有参构造
    explicit Socket(int fd) : fd_(fd) {};
    ~Socket();
    // 获取套接字
    int getFd() const { return fd_; };
    // 给套接字绑定ip和port
    void bindIpPort(const InetAddress& addr);
    // 给套接字绑设置监听状态
    void listenFd();
    // 接受客户端连接请求
    int acceptFd(InetAddress& perrAddr);
    // 关闭写端
    void shutdownWrite(bool on);
    // 设置端口复用
    void setPortReuse(bool on);
    // 设置sockaddr地址复用
    void setAddrReuse(bool on);
    // 设置连接心跳检测
    void setKeepAlive(bool on);
    // 设置nagle算法
    void setNodelay(bool on);

private:
    // 套接字
    int fd_;
};