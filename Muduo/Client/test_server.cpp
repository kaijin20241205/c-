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
using namespace std;

#include "EpollEventCallback.h"


uint64_t testTotal = 0;
int total_read_times = 0;
void task(int signum);

struct Client
{
    // 发送的总字节数
    uint64_t bytesWrite_;
    // 接收的总字节
    uint64_t bytesRead_;
    // 返回的epollevent
    vector<epoll_event> epvs_;
    // epoll
    int epfd_;
    // sockets
    vector<EpvCallback*> evpCallbacks_;;
};

class ClientsTest
{
public:
    Client* clients_;
    // 连接目标ip、port
    sockaddr_in addr_;
    // 发送消息的大小
    int messageSize_;
    // 每个客户端的并发数
    int sessionCount_;
    // 结束标志
    static bool shutdown_;

    ClientsTest(int sessionCount, 
                int messageSize,
                int clientNum,
                const string& ip = "127.0.0.1", 
                const uint16_t port = 8888)
    {
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr.s_addr);

        clients_ = new Client[clientNum];
        for (int i = 0; i < clientNum; i++)
        {
            clients_[i].epfd_ = epollCreate();
            clients_->epvs_.resize(sessionCount);
            clients_->bytesRead_ = 0;
            clients_->bytesWrite_ = 0;
            for (int j = 0; j < sessionCount; j++)
            {

            }
            
        }
    
        
    }
    EpvCallback* createEpvCallback()
    {
        int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (fd < 0)
        {
            cout << "socket error" << endl;
            return nullptr;
        }
        int optval = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
        
        int ret = connect(fd, (sockaddr*)&addr_, sizeof(addr_));
        if (ret == -1 && errno != EINPROGRESS)
        {
            cout << "connect fail" << " errno: " << errno << endl;
            close(fd);
            return nullptr;
        }

    }
};



// struct Client
// {
//     void (Client::*callback_)(int fd, int revents, void* data);
//     Client(int sessionCount, 
//             int messageSize,
//             const string& ip = "127.0.0.1", 
//             const uint16_t port = 8888)
//         : bytesWrite_(0)
//         , bytesRead_(0)
//         , sessionCount_(sessionCount)
//         , messageSize_(messageSize)
//         , epvs_(16)
//     {
//         epfd_ = epoll_create1(EPOLL_CLOEXEC);
//         addr_.sin_family = AF_INET;
//         addr_.sin_port = htons(port);
//         inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr.s_addr);
//         int j = 0;
//         int optval = 1;
//         for (int i = 0; i < sessionCount; i++)
//         {
//             int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
//             if (fd < 0)
//             {
//                 cout << "socket error" << endl;
//                 continue;
//             }
//             setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
            
//             int ret = connect(fd, (sockaddr*)&addr_, sizeof(addr_));
//             if (ret == -1 && errno != EINPROGRESS)
//             {
//                 cout << "connect fail" << " errno: " << errno << endl;
//                 close(fd);
//                 continue;
//             }
//             epoll_event epv{};
//             epv.data.fd = fd;
//             epv.events = EPOLLOUT | EPOLLERR;
//             epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &epv);
//             int n = epoll_wait(epfd_, &*epvs_.begin(), epvs_.size(), -1);
//             epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL);
//             if (n > 0)
//             {
//                 int error = 0;
//                 socklen_t len = sizeof(error);
//                 if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
//                 {
//                     perror("getsocket failed");
//                 }
//                 else if (error != 0)
//                 {
//                     cerr << strerror(error) << endl;
//                 }
//                 else
//                 {
//                     // cout << "connect successful..." << endl;
//                 }                
//             }
//             else
//             {
//                 cout << "epoll_wait error, errno: " << errno << endl;
//                 close(fd);
//                 continue;
//             }
            
//             EpvCallback* epvCallback = new EpvCallback;
//             memset(epvCallback, 0x00, sizeof(EpvCallback));
//             epvCallback->buf_ = new char[messageSize_];
//             memset(epvCallback->buf_, 'a', messageSize_);
//             epvCallback->bufLen_ = messageSize_;
//             epvCallback->callback_ = new (void (Client::*)(int, int, void*));
//             epvCallback->fd_ = fd;
//             epvCallback->evnets = EPOLLOUT;
//             *epvCallback->callback_ = &Client::writer;
//             epvCallback->bufIndex_ = messageSize_;

//             memset(&epv, 0x00, sizeof(epoll_event));
//             epv.data.ptr = reinterpret_cast<void*>(epvCallback);
//             epv.events = EPOLLOUT;
//             // 添加失败就一直添加直到添加成功
//             while (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &epv) < 0)
//             {
//                 cout << "epoll_ctl error" << endl;
//             }
//         }
//     }

//     void start()
//     {
//         struct sigaction sa;
//         struct itimerspec timer_spec;
//         timer_t timer_id;

//         sa.sa_handler = task;
//         sigaction(SIGALRM, &sa, NULL);

//         timer_create(CLOCK_REALTIME, NULL, &timer_id);
//         timer_spec.it_value.tv_sec = 1;
//         timer_spec.it_value.tv_nsec = 0;
//         timer_spec.it_interval.tv_sec = 1;
//         timer_spec.it_interval.tv_nsec = 0;



//         timer_settime(timer_id, 0, &timer_spec, NULL);

//         while (!shutdown_)
//         {
//             int num = epoll_wait(epfd_, &*epvs_.begin(), epvs_.size(), -1);
//             if (num > 0)
//             {
                
//                 auto it = epvs_.begin();
//                 for (int i = 0; i < num; i++, it++)
//                 {
//                     EpvCallback* epvCallback = reinterpret_cast<EpvCallback*>(it->data.ptr);
//                     this->callback_ = *epvCallback->callback_;
//                     (this->*callback_)(epvCallback->fd_, it->events, it->data.ptr);
//                 }
//             }
//         }

//         cout << "bytesRead_: " << bytesRead_ << endl;
//         cout << endl;
//         cout << static_cast<double>(bytesRead_) / (10 * 1024 * 1024) << " MiB/s throughput" << endl;
//         cout << static_cast<double>(total_read_times) / 10 << " request/s"<< endl;
        
//     }

//     void reader(int fd, int revents, void* data)
//     {
//         EpvCallback* epvCallback = reinterpret_cast<EpvCallback*>(data);

//         while (true)
//         {
//             int n = ::read(fd, epvCallback->buf_, epvCallback->bufLen_);
//             // 有数据可读
//             if (n > 0)
//             {
//                 bytesRead_ += n;
//                 epvCallback->bufIndex_ += n;
//             }
//             // 断开连接
//             else if (n == 0)
//             {
//                 delete[] epvCallback->buf_;
//                 epvCallback->bufLen_ = 0;
//                 delete epvCallback;
//                 epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL);
//                 close(fd);
//                 break;
//             }
//             else
//             {
//                 if ( errno == EAGAIN || errno == EWOULDBLOCK)
//                 {
//                     n = write(fd, epvCallback->buf_, epvCallback->bufIndex_);
//                     // test
//                     testTotal += n;
//                     total_read_times += 1;
//                     if (n > 0)
//                     {
//                         // 只发生了一部分，监听发送事件
//                         if (n < epvCallback->bufIndex_)
//                         {
//                             bytesWrite_ += n;
//                             epvCallback->bufIndex_ = epvCallback->bufIndex_ - n;
//                             *epvCallback->callback_ = &Client::writer;
//                             epvCallback->evnets = EPOLLOUT;
//                             epoll_event epv{};
//                             epv.data.ptr = data;
//                             epv.events = EPOLLOUT;
//                             epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &epv);
//                         }
//                         // 一次性全发出去，继续监听读事件
//                         else
//                         {
//                             epvCallback->bufIndex_ = 0;
//                             bytesWrite_ += n;
//                         }

//                         break;
//                     }
//                 }

//                 // 发生错误
//                 delete[] epvCallback->buf_;
//                 epvCallback->bufLen_ = 0;
//                 delete epvCallback;
//                 epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL);
//                 close(fd);
//                 break;
//             }
//         }
        

//     }

//     void writer(int fd, int revents, void* data)
//     {
//         EpvCallback* epvCallback = reinterpret_cast<EpvCallback*>(data);
//         int n = ::write(fd, epvCallback->buf_, epvCallback->bufIndex_);
//         if (n > 0)
//         {
//             // 发送一部分
//             if (n < epvCallback->bufIndex_)
//             {
//                 epvCallback->bufIndex_ = epvCallback->bufIndex_ - n;
//                 bytesWrite_ += n;
//             }
//             // 全部发送
//             else
//             {
//                 epvCallback->bufIndex_ = 0;
//                 epvCallback->evnets = EPOLLIN;
//                 bytesWrite_ += n;
//                 epoll_event epv{};
//                 epv.data.ptr = data;
//                 epv.events = EPOLLIN;
//                 *epvCallback->callback_ = &Client::reader;
//                 epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &epv); 
//             }

//         }
//         else
//         {
//             if (errno != EAGAIN || errno != EWOULDBLOCK)
//             {
//                 epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL);
//                 delete[] epvCallback->buf_;
//                 epvCallback->bufLen_ = 0;
//                 close(epvCallback->fd_);
//                 delete epvCallback;
//             }
//         }
//     }
//     // 发送的总字节数
//     uint64_t bytesWrite_;
//     // 接收的总字节
//     uint64_t bytesRead_;
//     // 客户端的并发数
//     int sessionCount_;
//     // 返回的epollevent
//     vector<epoll_event> epvs_;
//     int epfd_;
// };



int num = 0;

void task(int signum)
{
    cout << "task run..." << endl;
    num++;
    if (num == 10)
    {
        Client::shutdown_ = true;
    }
    cout << "testTotal: " << testTotal << endl;
    cout << "total_read_times: " << total_read_times << endl;
    
}

int main()
{
    Client cli(10, 16384);
    cli.start();
}


