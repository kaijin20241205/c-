#include <iostream>
#include <atomic>

#include "./include/gtest/gtest.h"
#include "./../include/Logger.hpp"
#include "./../include/Timestamp.hpp"
#include "./../include/InetAddress.hpp"
#include "./../include/Socket.hpp"

using namespace std;

// // std::atomic<int64_t> test(0);
// int test = 0;
// void* test_add(void* num)
// {
//     for (int i = 0; i < 100000000; i++)
//     {
//         // test.fetch_add(1, std::memory_order_seq_cst);
//         test++;
//     }
//     return NULL;
    
// }


int main(int argc, char** argv)
{
    // cout << "test Logger....................." << endl;

    // LOG_INFO("%s%d", "hello", 1);
    // LOG_ERROR("%s%d", "hello", 2);
    // // LOG_FATAL("%s%d", "hello", 2);
    // LOG_DEBUG("%s%d", "hello", 4);

    // LOG_DEBUG("%s%d", "hello", 5);
    // LOG_DEBUG("%s%d", "hello", 6);
    
    // cout << "test Timestamp......................." << endl;
    // cout << Timestamp::now().toString() << endl;
    // cout << Timestamp::now().toString() << endl;

    // cout << "test InetAddress.........................." << endl;
    // InetAddress addr1("127.0.0.1", 8888);
    // cout << addr1.getIp() << endl;
    // cout << addr1.getPort() << endl;
    // cout << addr1.getIpPort() << endl;
    // cout << addr1.getSockaddrIn() << endl;
    // InetAddress addr2(*addr1.getSockaddrIn());
    // cout << addr2.getIp() << endl;
    // cout << addr2.getPort() << endl;
    // cout << addr2.getIpPort() << endl;
    // cout << addr2.getSockaddrIn() << endl;


    // cout << "test Socket................." << endl;
    // int fd = socket(AF_INET, SOCK_STREAM, 0);
    // Socket s(fd);


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
    return RUN_ALL_TESTS();
    //test




    // test use atomic
    // pthread_t pthreads[50] = {};
    // for (int i = 0; i < 50; i++)
    // {
    //     pthread_create(&pthreads[i], NULL, test_add, NULL);
    // }
    // for (int i = 0; i < 50; i++)
    // {
    //     pthread_join(pthreads[i], NULL);
    // }

    // cout << test << endl;


}