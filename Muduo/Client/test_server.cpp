#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
using namespace std;

#include "EpollEventCallback.h"
#include "CallbackFunction.h"
#include "Buffer.h"
#include "EpollThreadPoll.h"
#include "MyError.h"


struct Client
{
    // 发送的总字节数
    uint64_t recieves_;
    // 接收的总字节
    uint64_t bytesRead_;
    // 返回的epollevent
    vector<epoll_event> epvs_;
    // epoll
    int epfd_;
    // sockets
    vector<EpvCallback*> evpCallbacks_;;
};

void countByteAndTimes(char *clientMsg, ssize_t clientMsgLen, Buffer *output, void* data)
{
    Client* client = reinterpret_cast<Client*>(data);
    client->bytesRead_ += clientMsgLen;
    client->recieves_++;
    bufferAppend(clientMsg, clientMsgLen, output);
}



class ClientsTest
{
public:
    // 客户端信息
    vector<Client> clients_;
    // 客户端线程池
    EpollThreadPoll* epollThreadPoll_;
    // 连接目标ip、port
    sockaddr_in addr_;
    // 发送消息的大小
    int messageSize_;
    string message_;
    // 每个客户端的并发数
    int sessionCount_;

    ClientsTest(int sessionCount, 
                int messageSize,
                int clientNum,
                const string& ip = "127.0.0.1", 
                const uint16_t port = 8888) : 
                                            sessionCount_(sessionCount), 
                                            messageSize_(messageSize), 
                                            message_("a", messageSize)
    {
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr.s_addr);

        clients_.resize(clientNum);
        for (int i = 0; i < clientNum; i++)
        {
            clients_[i].epfd_ = -1;
            clients_[i].epvs_.resize(sessionCount);
            clients_[i].bytesRead_ = 0;
            clients_[i].recieves_ = 0;
        }
        epollThreadPoll_ = epollThreadPollInit(clientNum);
    }
    
    void setFdLimit(rlim_t newLimit)
    {
        rlimit limit;
        if (getrlimit(RLIMIT_NOFILE, &limit) == -1)
        {
            errorExit(
                true, 
                "setFdLimit getrlimit error", 
                __FILE__, 
                __LINE__
            );
        }
        limit.rlim_cur = newLimit;
        if (newLimit > limit.rlim_max)
        {
            limit.rlim_max = newLimit;
        }

        if (setrlimit(RLIMIT_NOFILE, &limit) == -1)
        {
            errorExit(
                true, 
                "setFdLimit setrlimit error", 
                __FILE__, 
                __LINE__
            );
        }
        cout << "new limits: soft = " << limit.rlim_cur << ", hard = " << limit.rlim_max << endl;
    }

    int setSocketNonblock(int fd)
    {
        int flag = fcntl(fd, F_GETFL);
        flag |= O_NONBLOCK;
        flag = fcntl(fd, F_SETFD, flag);
        if (flag == -1)
        {
            return -1;
        }
        return fd;
    }

    vector<EpvCallback*> createEpvCallback(int epfd, Client* client)
    {
        vector<int> fds;
        for (int i = 0; i < sessionCount_; i++)
        {
            int fd = socket(AF_INET, SOCK_STREAM, 0);
            if (fd < 0)
            {
                cout << "socket error" << endl;
                continue;
            }
            int optval = 1;
            setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
            int ret = connect(fd, (sockaddr*)&addr_, sizeof(addr_));
            if (ret == -1 && errno != EINPROGRESS)
            {
                cout << "connect fail" << " errno: " << errno << endl;
                close(fd);
                continue;
            }
            fds.push_back(fd);
        }
        vector<EpvCallback*> evpCallbacks;
        for (int i = 0; i < fds.size(); i++)
        {
            setSocketNonblock(fds[i]);
            EpvCallback* epvCallback = epvCallbackInit(
                                                fds[i], 
                                                EPOLLOUT | EPOLLERR, 
                                                writer, 
                                                epfd, 
                                                NULL, 
                                                countByteAndTimes, 
                                                reinterpret_cast<void*>(client)
                                                );
            bufferAppend(message_.data(), message_.size(), epvCallback->outputBuf_);
            cout << "epvCallback: " << epvCallback->epfd_ << endl;
            epollAdd(reinterpret_cast<void*>(epvCallback));
            evpCallbacks.push_back(epvCallback);
        }
        return evpCallbacks;
    }

    void start()
    {
        int threadNum = epollThreadPoll_->threadNum_;
        epollThreadPollStart(epollThreadPoll_);
        for (int i = 0; i < threadNum; i++)
        {
            clients_[i].epfd_ = getNextSubThtreadEpfd(epollThreadPoll_);
            clients_[i].evpCallbacks_ = createEpvCallback(clients_[i].epfd_, &(clients_[i]));
        }

        int epfd = epollCreate();
        epoll_event epv;
        int times = 0;
        uint64_t totalByteRead = 0;
        uint64_t recieves_ = 0;
        while (times <= 10)
        {
            int n = epoll_wait(epfd, &epv, 1, 1000);
            times++;

            for (int i = 0; i < threadNum; i++)
            {
                totalByteRead += clients_[i].bytesRead_;
                recieves_ += clients_[i].recieves_;
            }
            cout << static_cast<double>(totalByteRead) / (times * 1024 * 1024) << " MiB/s throughput" << endl;
            cout << static_cast<double>(recieves_) / times << " recieve" << endl;
            totalByteRead = 0;
            recieves_ = 0;
        }
        close(epfd);
        for (int i = 0; i < threadNum; i++)
        {
            for (int j = 0; j < clients_[i].evpCallbacks_.size(); j++)
            {
                if (clients_[i].evpCallbacks_[j] != nullptr)
                {
                    cout << "clients_[" << i << "].evpCallbacks_[" << j << "]"<< endl;
                    // epvCallbackDestory(&(clients_[i].evpCallbacks_[j]));
                }
            }
        }

        epollThreadPollDestory(&epollThreadPoll_);
        
        for (int i = 0; i < threadNum; i++)
        {
            totalByteRead += clients_[i].bytesRead_;
            recieves_ += clients_[i].recieves_;
        }
        cout << static_cast<double>(totalByteRead) / (10 * 1024 * 1024) << " MiB/s throughput" << endl;
        cout << static_cast<double>(recieves_) / 10 << " recieve" << endl;
    }
};


int main()
{
    
    ClientsTest clientTest(1000, 16384, 2);
    clientTest.start();
}


