#include "./include/gtest/gtest.h"
#include "./../src/newDeauftPoller.cpp"
#include "./../include/Channel.hpp"
#include "./../include/Socket.hpp"
#include "./../include/InetAddress.hpp"
#include "./../include/Timestamp.hpp"

#include <iostream>
using namespace std;


class EventLoop
{

};

void testWriteCallback()
{
    cout << "testWriteCallback" << endl;
}

TEST(Epoller, simple_test)
{
    EventLoop e;
    Poller* p = Poller::newDeauftPoller(&e);
    InetAddress a("127.0.0.1", 8888);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    Socket s(fd);
    s.bindIpPort(a);
    s.listenFd();
    Channel c(s.getFd(), &e);

    EXPECT_FALSE(p->hasChannel(&c));
    p->updateChannel(&c);
    EXPECT_TRUE(p->hasChannel(&c));
    std::vector<Channel*> channelList;
    Timestamp times = p->loop(5000, &channelList);
    cout << times.toString() << endl;
    p->removeChannel(&c);
    EXPECT_FALSE(p->hasChannel(&c));
}


