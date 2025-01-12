#include <iostream>

#include "./include/gtest/gtest.h"
#include "./../include/Logger.hpp"
#include "./../include/Timestamp.hpp"
#include "./../include/InetAddress.hpp"
#include "./../include/Socket.hpp"

using namespace std;


int main(int argc, char** argv)
{
    cout << "test Logger....................." << endl;

    LOG_INFO("%s%d", "hello", 1);
    LOG_ERROR("%s%d", "hello", 2);
    // LOG_FATAL("%s%d", "hello", 2);
    LOG_DEBUG("%s%d", "hello", 4);

    LOG_DEBUG("%s%d", "hello", 5);
    LOG_DEBUG("%s%d", "hello", 6);

    cout << "test Timestamp......................." << endl;
    cout << Timestamp::now().toString() << endl;
    cout << Timestamp::now().toString() << endl;

    cout << "test InetAddress.........................." << endl;
    InetAddress addr1("127.0.0.1", 8888);
    cout << addr1.getIp() << endl;
    cout << addr1.getPort() << endl;
    cout << addr1.getIpPort() << endl;
    cout << addr1.getSockaddrIn() << endl;
    InetAddress addr2(*addr1.getSockaddrIn());
    cout << addr2.getIp() << endl;
    cout << addr2.getPort() << endl;
    cout << addr2.getIpPort() << endl;
    cout << addr2.getSockaddrIn() << endl;


    cout << "test Socket................." << endl;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    Socket s(fd);


    // cout << s.getFd() << endl;
    // s.setAddrReuse(true);
    // s.setPortReuse(true);
    // s.setKeepAlive(true);
    // s.setNodelay(true);
    // s.bindIpPort(addr1);
    // s.listenFd();
    // Socket cfd = Socket(s.acceptFd(addr2));     // nc 127.0.0.1 8888
    // cout << addr2.getIpPort() << endl;
    // cout << cfd.getFd() << endl;
    // // cfd.shutdownWrite(true);     //测试成功 pipe broke
    // write(cfd.getFd(), "hello", 6);
    // 初始化googletest
    ::testing::InitGoogleTest(&argc, argv);
    cout << "test..." << endl;
    RUN_ALL_TESTS();
    //test
    return 0;
}