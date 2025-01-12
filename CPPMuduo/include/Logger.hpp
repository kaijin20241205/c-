#pragma once

#include <string>

#include "noncopyable.hpp"

#define MUDUO_DEBUG


#define LOG_INFO(logmsgFormat, ...) \
    do \
    { \
        Logger& log = Logger::instance(); \
        log.setLogger(INFO); \
        char buf[1024]{}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        log.log(buf); \
    } while (0)

#define LOG_ERROR(logmsgFormat, ...) \
    do \
    { \
        Logger& log = Logger::instance(); \
        log.setLogger(ERROR); \
        char buf[1024]{}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        log.log(buf); \
    } while (0)

#define LOG_FATAL(logmsgFormat, ...) \
    do \
    { \
        Logger& log = Logger::instance(); \
        log.setLogger(FATAL); \
        char buf[1024]{}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        log.log(buf); \
        exit(-1); \ 
    } while (0)

#ifdef MUDUO_DEBUG
#define LOG_DEBUG(logmsgFormat, ...) \
    do \
    { \
        Logger& log = Logger::instance(); \
        log.setLogger(DEBUG); \
        char buf[1024]{}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        log.log(buf); \
    } while (0)
#else
    // 没有开启debug模式，使用LOG_DEBUG时，什么都不做
    #define LOG_DEBUG(logmsgFormat, ...)
#endif


// 定义日志的级别
enum LoggerLevel
{
    // 普通信息
    INFO,
    // 错误信息
    ERROR,
    // 程序奔溃信息
    FATAL,
    // 调试信息
    DEBUG,
};


// 将Logger类设计成单例
class Logger : noncopyable
{
public:
    // 获取Logger类单例的方法
    static Logger& instance();
    // 设置Logger的级别
    void setLogger(int level);
    // 打印日志
    void log(std::string msg);
private:
    // 将构造函数设置称私有
    Logger() = default;
    // Logger的级别
    int loggerLevel_;
};