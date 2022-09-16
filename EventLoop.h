//
// Created by gnezd on 2022-08-20.
//

#ifndef DDMUDUO_EVENTLOOP_H
#define DDMUDUO_EVENTLOOP_H

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

class Channel;
class Poller;

// 时间循环类
// 主要包含两个大模块 Channel Poller(epoll的抽象)
class EventLoop {
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const {return pollReturnTime_;}

    // 在当前loop中执行cb
    void runInLoop(Functor cb);
    // 将cb放入队列中 唤醒loop所在的线程 执行cb
    void queueInLoop(Functor cb);

    // 唤醒loop所在的线程
    void wakeup();

    // EventLoop的方法==》poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断eventLoop对象是否在自己的线程中
    bool isInLoopThread() const {return threadId_ == CurrentThread::tid();}

private:
    void handleRead();
    // 执行回调
    void doPendingFunctors();
    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_;
    std::atomic_bool quit_; // 退出loop循环
    const pid_t threadId_; // 记录当前loop所在的线程id
    Timestamp pollReturnTime_; // poller返回事件的时间点
    std::unique_ptr<Poller> poller_;

    // 当mainLoop获取一个新用户的channel
    // 通过轮询算法选择一个subloop
    // 通过该成员唤醒subloop处理
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;
    Channel *currentActiveChannel_;

    std::atomic_bool callingPendingFunctors_; // 当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_; // 存储loop需要执行的所有回调操作
    std::mutex mutex_; // 互斥锁 用于保护上面vector容器的线程安全


};

#endif //DDMUDUO_EVENTLOOP_H
