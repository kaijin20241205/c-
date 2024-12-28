#ifndef MY_MEMORY_HOOK
#define MY_MEMORY_HOOK

#include <stdlib.h>
#include <stdio.h>

// 记录内存分配信息
typedef struct MemRecord {
    void *address;
    size_t size;
    struct MemRecord *next;
} MemRecord;

extern MemRecord *head;

// 内存分配记录函数
void add_record(void *address, size_t size);
void remove_record(void *address);
void check_leaks();

// 自定义 malloc 和 free
void *my_malloc(size_t size);
void my_free(void *ptr);

// 宏替换 malloc 和 free
#define malloc(size) my_malloc(size)
#define free(ptr) my_free(ptr)

#endif // MEMORY_HOOK_H
