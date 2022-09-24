//
// Created by gnezd on 2022-08-21.
//

#include "EPollPoller.h"
#include "Channel.h"
#include "Logging.h"

#include <errno.h>
#include <unistd.h>
#include <string.h>

// channel未添加到poller中
const int kNew = -1; // channel成员的index_ = -1
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize) {
    if (epollfd_ < 0) LOG << "epoll_create error:" << errno;
}

EPollPoller::~EPollPoller() noexcept {
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels) {

    int numEvents = ::epoll_wait(epollfd_,&(*events_.begin()),static_cast<int>(events_.size()),timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if (numEvents > 0) {
        LOG << numEvents << "events happened";
        fillActiveChannels(numEvents,activeChannels);
        if (numEvents == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {
        LOG << "timeout!";
    } else {
        // 不是外部中断 ???
        if (saveErrno != EINTR) {
            errno = saveErrno;
            LOG << "EPollPoller::poll() err!";
        }
    }
    return now;
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) {
    for(int i = 0; i < numEvents; i++) {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); // EventLoop就拿到了其poller给它返回发生事件的channel列表
    }
}

void EPollPoller::updateChannel(Channel *channel) {
    const int index = channel->index();

    if (index == kNew || index == kDeleted) {
        if (index == kNew) {
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD,channel);
    } else {
        // channel已经在poller上注册过
        int fd = channel->fd();
        if (channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);
        } else {
            update(EPOLL_CTL_MOD,channel);
        }
    }
}

void EPollPoller::update(int operation, Channel *channel) {
    epoll_event event;
    memset(&event,0, sizeof(event));
    int fd = channel->fd();
    event.data.fd = fd;
    event.events = channel->events();
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_,operation,fd,&event) < 0) {
        if (operation == EPOLL_CTL_DEL) LOG << "epoll_ctl del error:" << errno;
        else LOG << "epoll_ctl add/mod error:" << errno;
    }
}

// 从poller中删除channel
void EPollPoller::removeChannel(Channel *channel) {
    int fd = channel->fd();
    channels_.erase(fd);

    int index = channel->index();
    if (index == kAdded) update(EPOLL_CTL_DEL,channel);
    channel->set_index(kNew);
}