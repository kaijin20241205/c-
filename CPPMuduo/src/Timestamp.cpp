#include "./../include/Timestamp.hpp"

#include <sys/time.h>   // timeval
#include <time.h>       // localtime
#include <iostream>


// 返回一个当前时间（精度为为微秒的）Timestamp对象
Timestamp Timestamp::now()
{
    // timeval结构体的精度是微妙
    timeval tv{};
    // 获得当前时间精确到微妙
    gettimeofday(&tv, nullptr);
    int64_t microSecond = (tv.tv_sec * secondRate_) + tv.tv_usec;
    // 返回一个当前时间（精度为为微秒的）Timestamp对象
    return Timestamp(microSecond);
}

// 将Timestamp对象中存储的当前时间转换称一个符合人类阅读习惯格式的字符串并返回
std::string Timestamp::toString() const
{
    // 获得当前时间精度为秒并转换称time_t格式
    time_t second = static_cast<time_t>(microsecondSinceEpoch_ / secondRate_);
    // 将当前时间转换成年月日时分秒格式的结构体
    tm* tm_time = localtime(&second);
    // 将时间转换称年月日时分秒.微秒形式的字符串
    char buf[64]{};
    // 获取当前时间的微秒部分的数值并转换成int形式
    int microsecond = static_cast<int>(microsecondSinceEpoch_ % secondRate_);
    // 将时间按照年月日时分秒.微秒的形式输出到buf中
    snprintf(buf, 64, "%4d-%02d-%02d %02d:%02d:%02d.%06d", 
        tm_time->tm_year + 1900, 
        tm_time->tm_mon + 1, 
        tm_time->tm_mday, 
        tm_time->tm_hour, 
        tm_time->tm_min, 
        tm_time->tm_sec,
        // 多少毫秒
        microsecond
        );
    // 将buf作为字符串返回
    return buf;
}

