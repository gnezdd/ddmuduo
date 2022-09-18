//
// Created by gnezd on 2022-09-18.
//

#include "AsyncLogging.h"

AsyncLogging::AsyncLogging(const std::string basename, int flushInterval)
    : flushInterval_(flushInterval)
    , running_(false)
    , basename_(basename)
    , thread_(std::bind(&AsyncLogging::threadFunc,this),"Logging")
    , mutex_()
    , cond_()
    , currentBuffer_(new Buffer)
    , nextBuffer_(new Buffer)
    , buffers_()
    , latch_(1) {
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}

void AsyncLogging::append(const char *logline, int len) {
    std::unique_lock<std::mutex> lock(mutex_);
    // 如果当前缓冲区可以写入
    if (currentBuffer_->avail() > len) {
        currentBuffer_->append(logline,len);
    } else {
        // 将当前缓冲区添加到缓冲队列中 将预备缓冲区作为当前缓冲区
        buffers_.push_back(currentBuffer_);
        currentBuffer_.reset();
        if (nextBuffer_) currentBuffer_ = std::move(nextBuffer_);
        else currentBuffer_.reset(new Buffer);
        currentBuffer_->append(logline,len);
        cond_.notify_one();
    }
}

void AsyncLogging::threadFunc() {
    latch_.countDown();

}
