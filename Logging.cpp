//
// Created by gnezd on 2022-09-18.
//

#include "Logging.h"
#include "AsyncLogging.h"
#include "iostream"

#include <time.h>
#include <sys/time.h>

// 在多线程中有些事情只需要执行一次
// 通常初始化应用程序时可以将其放在main中
// 但写一个库时就不行了
// 可以用静态初始化 使用一次初始化pthread_once就可以了
// 多线程中尽管pthread_once会调用出现在多个线程中 once_init只被执行一次
// 有那个线程执行是由内核调度的
static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static AsyncLogging *AsyncLogger_;

std::string Logger::logFileName_ = "./ddmuduo.log";

// 启动刷新日志 在多线程环境中只会启动一次
void once_init() {
    AsyncLogger_ = new AsyncLogging(Logger::getLogFileName());
    AsyncLogger_->start();
}

void output(const char* msg,int len) {
    // 启动刷新日志
    pthread_once(&once_control,once_init);
    // 将日志添加到日志文件中
    AsyncLogger_->append(msg,len);
}

Logger::Impl::Impl(const char *fileName, int line)
    : stream_()
    , line_(line)
    , baseName(fileName) {
    formatTime();
}

void Logger::Impl::formatTime() {
    struct timeval tv;
    time_t time;
    char str_t[26] = {0};
    gettimeofday(&tv,NULL);
    time = tv.tv_sec;
    struct tm* p_time = localtime(&time);
    strftime(str_t,26,"%Y-%m-%d %H:%M:%S\n", p_time);
    stream_ << str_t;
}

// 初始化时添加时间
Logger::Logger(const char *fileName, int line)
    : impl_(fileName,line)
{}

// 析构时添加文件与行数信息 并输出到后端中
Logger::~Logger() {
    impl_.stream_ << "--" << impl_.baseName << ':' << impl_.line_ << '\n';
    const LogStream::Buffer &buf(stream().buffer());
    output(buf.data(),buf.length());
}


