#include "./../include/Logger.hpp"
#include "./../include/Timestamp.hpp"

#include <iostream>


Logger& Logger::instance()
{
    // 局部静态变量的初始化语句只有在第一次初始化时会生效
    static Logger logger;
    return logger;
}

// 设置日志级别
void Logger::setLogger(int level)
{
    loggerLevel_ = level;
}

// 打印日志
void Logger::log(std::string msg)
{
    switch (loggerLevel_)
    {
    case INFO:
        std::cout << "[INFO] :";
        break;
    case DEBUG:
        std::cout << "[DEBUG]:";
        break;
    case ERROR:
        std::cout << "[ERROR]:";
        break;
    case FATAL:
        std::cout << "[FATAL]:";
        break;
    default:
        break;
    }
    // todo class Timestamp
    std::cout << Timestamp::now().toString() << " " << msg << std::endl;
}

