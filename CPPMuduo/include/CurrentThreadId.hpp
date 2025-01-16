#pragma once

namespace CurrentThread
{
    /*
    thread_local:  C++11 引入的关键字，用于声明 线程局部存储（Thread-Local Storage, TLS） 
    的变量。每个线程对 thread_local 声明的变量都有自己的独立副本，这些副本互不干扰，
    */
    extern thread_local int t_cachedThreadId;
    // 缓存线程id
    void cachedThreadid();
    // 获取线程id
    inline int tid()
    {
        /*
        __builtin_expect：是 GCC 提供的内置函数，用于分支预测优化。
        __builtin_expect(x, expected) 的作用是告诉编译器：x 的值更有可能是 expected，以便编译器优化代码路径。
        __builtin_expect(t_cachedThreadId == 0, 0)表示 t_cachedTid == 0 不太可能发生，因此编译器将这一分支放在
        不常执行的路径中。这样做的原因是，t_cachedTid 通常会在第一次调用时初始化，之后直接返回缓存的值。
        */
        if (__builtin_expect(t_cachedThreadId == 0, 0))
        {
            cachedThreadid();
        }
        return t_cachedThreadId;
    }
}