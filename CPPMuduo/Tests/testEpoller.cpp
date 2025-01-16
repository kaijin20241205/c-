#include "./include/gtest/gtest.h"
#include "./../src/newDeauftPoller.cpp"
#include "./../include/Channel.hpp"
#include "./../include/Socket.hpp"
#include "./../include/InetAddress.hpp"
// #include "./../include/Timestamp.hpp"
#include "./../include/EventLoop.hpp"

#include <fcntl.h>
#include <sys/timerfd.h>
#include <iostream>
using namespace std;

Channel* l_channel;
Channel* c_channel;
EventLoop* e_ptr;


void testCloseCallback()
{
    cout << "testCloseCallback" << endl;
    c_channel->setWriteEventCallback(nullptr);
    c_channel->setDisableAll();
    close(c_channel->getFd());
}

void testWriteCallback()
{
    cout << "testWriteCallback" << endl;
    int n = write(c_channel->getFd(), "hello", 6);
    cout << "write:" << n << endl;
    cout << "errno: " << errno << endl;
    c_channel->setCloseEventCallback(testCloseCallback);
    int cfd = c_channel->getFd();
    
    if (shutdown(cfd, SHUT_RDWR) < 0)
    {
        cout << "shutdown SHUT_RDWR error" << endl;
    }
}

void testReadCallback(Timestamp timestamp)
{
    cout << "testReadCallback" << endl;
    char buf[1024] = {};
    int n = read(c_channel->getFd(), buf, 1024);
    if (n == 0)
    {
        cout << ":)" << endl;
        close(c_channel->getFd());
    }
    else if (n > 0)
    {
        cout << buf << endl;
        c_channel->setWriteEventCallback(testWriteCallback);
        c_channel->setEnableWriting();
        c_channel->setDisableReading();
    }


}

void testAcceptCallback(Timestamp timestamp)
{
    cout << timestamp.toString() << endl;
    cout << "testAcceptCallback" << endl;
    int fd = l_channel->getFd();
    int cfd = accept4(fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
    cout << cfd << endl;
    c_channel = new Channel(cfd, e_ptr);
    c_channel->setReadEventCallback(testReadCallback);
    c_channel->setEnableReading();
}

void timeout(Timestamp time)
{
    cout << "timeout" << endl;
    e_ptr->quit();
}


TEST(Epoller, base_test)
{
    EventLoop e;
    e_ptr = &e;
    InetAddress a("127.0.0.1", 8888);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    Socket s(fd);
    s.setAddrReuse(true);
    s.bindIpPort(a);
    s.listenFd();
    l_channel = new Channel(s.getFd(), &e);
    l_channel->setReadEventCallback(testAcceptCallback);
    l_channel->setEnableReading();
    EXPECT_TRUE(e.hasChannel(l_channel));

    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    Channel channle(timerfd, e_ptr);
    channle.setReadEventCallback(timeout);
    channle.setEnableReading();
    itimerspec howlong = {};
    howlong.it_value.tv_sec = 10;

    ::timerfd_settime(timerfd, 0, &howlong, NULL);

    e.loop();
}




