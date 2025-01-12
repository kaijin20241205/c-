#include "./../include/InetAddress.hpp"

#include <strings.h>        // bzero

// 有参构造
InetAddress::InetAddress(const std::string ip, uint16_t port)
{
    bzero(&addrIn_, sizeof(addrIn_));
    addrIn_.sin_family = AF_INET;
    addrIn_.sin_port = htons(port);
    inet_pton(AF_INET, ip.data(), &addrIn_.sin_addr.s_addr);
}

// 获得端口
uint16_t InetAddress::getPort() const
{
    return ntohs(addrIn_.sin_port);
}

// 获得ip
std::string InetAddress::getIp() const
{
    char buf[16] = {};
    socklen_t len = sizeof(buf);
    inet_ntop(AF_INET, &addrIn_.sin_addr.s_addr, buf, len);
    return buf;
}

// 获得ip、port
std::string InetAddress::getIpPort() const
{
    return getIp() + ":" + std::to_string(getPort());
}
