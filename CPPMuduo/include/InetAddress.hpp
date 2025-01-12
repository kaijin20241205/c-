#pragma once
#include "./copyable.hpp"

#include <arpa/inet.h>      // sockaddr_in、inet_pton、inet_ntop
#include <string>



class InetAddress : public copyable
{
public:
    // 有参构造
    explicit InetAddress(const std::string ip, uint16_t port);
    // 有参构造
    explicit InetAddress(const sockaddr_in& addrIn) : addrIn_(addrIn) {};
    // 获取sockaddr_in地址
    const sockaddr_in* getSockaddrIn () const { return &addrIn_; };
    // 获得端口
    uint16_t getPort() const;
    // 获得ip
    std::string getIp() const;
    // 获得ip、port
    std::string getIpPort() const;
    // 设置sockaddr_in
    void setAddrIn(const sockaddr_in& addrIn) { addrIn_ = addrIn; };
private:
    sockaddr_in addrIn_;
};