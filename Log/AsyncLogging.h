//
// Created by gnezd on 2022-09-18.
//

#ifndef DDMUDUO_ASYNCLOGGING_H
#define DDMUDUO_ASYNCLOGGING_H

#include <memory>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "LogStream.h"
#include "Thread.h"
#include "CountDownLatch.h"

// 实现异步日志 将日志从缓冲区中刷新到磁盘
// addcode noncopyable
class AsyncLogging {
public:
    AsyncLogging(const std::string basename,int flushInterval  = 2);
    ~AsyncLogging() {
        if (running_) stop();
    }

    // 往日志缓冲区中添加日志
    void append(const char* logline,int len);

    // 开始刷新日志
    void start() {
        running_ = true;
        thread_.start();
        latch_.wait();
    }

    // 关闭刷新日志
    void stop() {
        running_ = false;
        cond_.notify_one();
        thread_.join();
    }

private:
    // 刷新日志
    void threadFunc();

    using Buffer = FixedBuffer<kLargeBuffer>; // 4M
    using BufferPtr = std::shared_ptr<Buffer>;
    using BufferVector = std::vector<std::shared_ptr<Buffer>>;

    // 当前缓冲区 Logging将内容先缓存在这里
    BufferPtr currentBuffer_;
    // 预备缓冲区 append向当前缓冲区添加日志消息时 如果当前缓冲不够
    // 当前缓冲区会被移动到buffers_缓冲队列中 预备缓冲区作为新的当前缓冲
    BufferPtr nextBuffer_;
    // 存储准备写入到后端的缓冲区 刷新时就是将buffers中的内容刷新到磁盘中
    BufferVector buffers_;

    const int flushInterval_; // 设置多久刷新一次
    bool running_;
    std::string basename_;
    Thread thread_; // 负责将缓冲区内容刷新到磁盘
    std::mutex mutex_;
    std::condition_variable cond_;
    CountDownLatch latch_;


};

#endif //DDMUDUO_ASYNCLOGGING_H
