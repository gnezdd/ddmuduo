//
// Created by gnezd on 2022-08-20.
//

#include "EventLoop.h"
#include "Logging.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

// 防止一个线程创建多个EventLoop
// 线程局部存储TLS 每个线程都有一个该变量的实例
// 可以通过该值判断该线程是否已经创建了EventLoop
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd 用来notify唤醒subReactor处理新来的fd
int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) LOG << "eventfd error:" << errno;
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this,wakeupFd_)) {
    if (t_loopInThisThread) {
        // addcode FATAL日志 需要退出
        LOG << "Another EventLoop:" << t_loopInThisThread <<  "exists in this thread:" << threadId_;
    } else {
        t_loopInThisThread = this;
    }

    // 设置wakeup的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
    // 每个eventLoop都将监听wakeupchannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

// 开启事件循环
void EventLoop::loop() {
    looping_ = true;
    quit_ = false;

    LOG << "EventLoop:" << this << "start looping";

    while(!quit_) {
        activeChannels_.clear();
        // 监听两种fd client的fd wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs,&activeChannels_);
        for (Channel * channel : activeChannels_) {
            // poller监听哪些channel发生了事件上报给eventloop
            // eventloop通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前eventLoop事件循环需要处理的回调操作
        // IO线程 mainLoop accept fd<=channel subloop
        // mainloop事先注册一个回调cb 需要subloop执行
        // wakeup subloop后执行下面方法执行之前mainloop注册的cb操作
        doPendingFunctors();
    }
    LOG << "EventLoop:" << this << "stop looping.";
    looping_ = false;
}

// 退出事件循环 1.loop在自己的线程中调用quit 2.在非loop的线程中调用loop的quit
void EventLoop::quit() {
    quit_ = true;

    // 如果是在其他线程中通过调用quit
    // 在一个subloop中调用了main的quit
    if (!isInLoopThread()) {
        wakeup();
    }
}

// 在当前loop中执行cb
void EventLoop::runInLoop(Functor cb) {
    // 在当前loop线程中执行cb
    if (isInLoopThread()) cb();
    // 在非当前loop线程中执行cb 需要唤醒loop所在的线程 执行cb
    else {
        queueInLoop(cb);
    }
}

// 将cb放入队列中 唤醒loop所在的线程 执行cb
void EventLoop::queueInLoop(Functor cb) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    // 唤醒相应的 需要执行上面回调操作的loop线程
    // || callingPendingFunctors_表示当前loop正在执行回调 但是loop又有了新的回调
    if (!isInLoopThread() || callingPendingFunctors_) wakeup(); // 唤醒loop所在的线程
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_,&one,sizeof one);
    if (n != sizeof one) {
        LOG << "EventLoop::handleRead() reads" << n << "bytes instead of 8";
    }
}

// 唤醒loop所在的线程
// 向wakeupfd_写一个数据 wakeupChannel就发生读事件
// 当前loop线程就会被唤醒
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_,&one,sizeof one);
    if (n != sizeof one) {
        LOG << "EventLoop::wakeup() writes" << n << "bytes instead of 8";
    }
}

void EventLoop::updateChannel(Channel *channel) {
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel) {
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functor : functors) {
        functor(); // 执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}