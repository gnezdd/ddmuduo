//
// Created by gnezd on 2022-08-22.
//

#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc,this),name)
    , mutex_()
    , cond_()
    , callback_(cb)
{}

EventLoopThread::~EventLoopThread() {
    exiting_ = false;
    if (loop_ != nullptr) {
        loop_->quit();
        thread_.join();// ???
    }
}

EventLoop *EventLoopThread::startLoop() {
    thread_.start(); // 启动底层的新线程

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr) cond_.wait(lock);
        loop = loop_;
    }
    return loop;
}

// 在单独的新线程中运行的 ???
void EventLoopThread::threadFunc() {
    // 创建一个独立的eventLoop 和上面的线程一一对应
    EventLoop loop;

    if (callback_) {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}