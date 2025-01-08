#pragma once
#include "./copyable.hpp"

#include <cstdint>
#include <string>


// 时间戳类
class Timestamp : public copyable
{
public:
    // 无参构造
    Timestamp() : microsecondSinceEpoch_(0) {};
    
    // 有参构造-禁止隐式类型转换
    explicit Timestamp(int64_t microsecondSinceEpoch) : microsecondSinceEpoch_(microsecondSinceEpoch) {};
    
    // 获得一个当前时间的Timestamp类
    static Timestamp now();
    
    // 返回一个符合人类阅读习惯的时间戳格式，该函数为常函数
    std::string toString() const;
private:
    // 从新纪元开始到现在为止的微秒数
    // 为什么用int64_t而不是uint64_t？在1970年1月1日之前的数据可以用负数来表示
    int64_t microsecondSinceEpoch_;
    
    /* 
        微秒与秒的进率：
        为什么使用const修饰？
        1、这个变量代表的意义是秒到微秒的进率，后续无需修改
        2、将静态成员变量通过const修饰后该变量为常量，常量在编译期就能确定值，常量存放在.rodata（只读数据段）
        段而不是运行时的堆、栈区，C++11及以后，static修饰的整数、枚举类型的静态成员变量可以直接类内初始化而
        无需类内声明类外初始化，更加简洁方便
    */
    static const int secondRate_ = 1000 * 1000;
};