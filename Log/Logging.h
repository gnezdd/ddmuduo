//
// Created by gnezd on 2022-09-18.
//

#ifndef DDMUDUO_LOGGING_H
#define DDMUDUO_LOGGING_H

#include "LogStream.h"

// 日志输出类
// 应该有一个LogStream的实例存储日志消息 Impl类

class Logger {
public:
    Logger(const char *fileName,int line);
    ~Logger();

    // 获取日志消息
    LogStream& stream() {return impl_.stream_;}

    static void setLogFileName(std::string fileName) {logFileName_ = fileName;}
    static std::string getLogFileName() {return logFileName_;}

private:
    class Impl {
    public:
        Impl(const char *fileName,int line);
        void formatTime();

        LogStream stream_;
        int line_;
        std::string baseName;
    };
    // 存储日志消息
    Impl impl_;
    static std::string logFileName_;
};

// 日志输出的宏定义
#define LOG Logger(__FILE__,__LINE__).stream()

#endif //DDMUDUO_LOGGING_H
