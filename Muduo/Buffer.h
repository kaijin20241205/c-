#pragma once
// #include <stddef.h>      size_t
#include <stdio.h>

struct Buffer
{
    // 缓存区
    char* buffer_;
    // Buffer头部预留容量
    ssize_t headSpace_;
    // Buffer总容量
    ssize_t capacity_;
    // 读指针缓冲区指针（从什么位置开始读）
    ssize_t readIndex_;
    // 写指针缓冲区指针（从什么位置开始写）
    ssize_t writeIndex_;
};

// 初始化Buffer
Buffer* bufferInit(ssize_t headSize = 8, ssize_t bodySize = 4096);

// 回收Buffer
void bufferDestory(Buffer** buf);

// 可读字节
ssize_t bufferReadableBytes(const Buffer* buf);

// 可写字节
ssize_t bufferWriteableBytes(const Buffer* buf);

// 预留缓冲区（可读区域中已经读走的区域）
ssize_t bufferPrependBytes(const Buffer* buf);

// 为buffer扩容n个字节的容量
void bufferExpandByest(ssize_t n, Buffer* buf);

// 检查buffer的可写容量是否足够写入
void ensureWriteBytes(ssize_t len, Buffer* buf);

// 向buf添加数据
void bufferAppend(const char* data, ssize_t dataLen, Buffer* buf);

// 从套接字中读取数到buf
size_t bufferReadFd(int fd, Buffer* buf, int* errorNum);

// 读取buffer中所有可读数据
void bufferRetrieveAll(Buffer* buf);

// 返回buffer中所有可读数据
char* bufferRetrieveAllAsString(Buffer* buf);

// 读取buffer中n个字节的可读数据
void bufferRetrieve(ssize_t n, Buffer* buf);

// 返回buffer中n个字节的可读数据
char* bufferRetrieveAsString(ssize_t n, Buffer* buf);