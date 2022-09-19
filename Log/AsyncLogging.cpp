//
// Created by gnezd on 2022-09-18.
//

#include "AsyncLogging.h"
#include "LogFile.h"
#include "iostream"

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
    LogFile output(basename_);
    BufferPtr newBuffer1(new Buffer); // 用于填充移动后的currentBuffer_
    BufferPtr newBuffer2(new Buffer); // 用于填充使用后的nextBuffer_
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite; // 后端缓冲区队列
    buffersToWrite.reserve(16);
    while (running_) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            // 如果缓冲队列中为空则等待
            // 批处理 积累到缓冲区满了就刷新
            // 如果没满但是时间到了也刷新
            if (buffers_.empty()) {
                // 阻塞等待缓冲队列有数据或时间到了
                cond_.wait_for(lock,std::chrono::seconds(flushInterval_));
            }
            // 将当前缓冲区添加到缓冲队列中
            // 重置当前缓冲区
            buffers_.push_back(currentBuffer_);

            currentBuffer_ = std::move(newBuffer1);

            buffersToWrite.swap(buffers_);
            if (!nextBuffer_) nextBuffer_ = std::move(newBuffer2);
        }

        // 如果最后后端缓冲区的缓冲区太多就只保留前三个
        if (buffersToWrite.size() > 25) {
            buffersToWrite.erase(buffersToWrite.begin()+2,buffersToWrite.end());
        }

        // 将缓冲队列中的内容刷新到日志文件中
        for (size_t i = 0; i < buffersToWrite.size(); i++) {
            output.append(buffersToWrite[i]->data(),buffersToWrite[i]->length());
        }

        // 此时后端缓冲区中的日志都已经全部写出了 就可以重置了
        if (buffersToWrite.size() > 2) {
            buffersToWrite.resize(2);
        }

        if (!newBuffer1) {
            newBuffer1 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if (!newBuffer2) {
            newBuffer2 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}
