#ifndef BUFFER
#define BUFFER
// #include <stddef.h>      size_t
#include <stdio.h>

struct Buffer
{
    // 缓存区
    char* buffer_;
    // Buffer头部预留容量
    size_t headSpace_;
    // Buffer总容量
    size_t capacity_;
    // 读指针缓冲区指针（从什么位置开始读）
    size_t readIndex_;
    // 写指针缓冲区指针（从什么位置开始写）
    size_t writeIndex_;
};

// 初始化Buffer
Buffer* bufferInit(size_t headSize = 8, size_t bodySize = 4096);

// 回收Buffer
void bufferDestory(Buffer** buf);

// 可读字节
size_t bufferReadableBytes(const Buffer* buf);

// 可写字节
size_t bufferWriteableBytes(const Buffer* buf);

// 预留缓冲区（可读区域中已经读走的区域）
size_t bufferPrependBytes(const Buffer* buf);

// 为buffer扩容n个字节的容量
void bufferExpandByest(size_t n, Buffer* buf);

// 检查buffer的可写容量是否足够写入len字节的数据
void ensureWriteBytes(size_t len, Buffer* buf);

// 向buf添加数据
void bufferAppend(const char* data, size_t dataLen, Buffer* buf);

// 读取buffer中所有可读数据
void bufferRetrieveAll(Buffer* buf);

// 返回buffer中所有可读数据
char* bufferRetrieveAllAsString(Buffer* buf);

// 读取buffer中n个字节的可读数据
void bufferRetrieve(size_t n, Buffer* buf);

// 返回buffer中n个字节的可读数据
char* bufferRetrieveAsString(size_t n, Buffer* buf);

// 从套接字中读取数到buf
ssize_t bufferReadFd(int fd, Buffer* buf, int* errorNum);

// 从buffer写入数据到套接字中
ssize_t bufferWriteFd(int fd, Buffer* buf, int* errorNum);

#endif