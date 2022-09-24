//
// Created by gnezd on 2022-08-24.
//

#include "TcpConnection.h"
#include "Logging.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

static EventLoop* checkLoopNotNull(EventLoop *loop) {
    if (loop == nullptr) {
        LOG << "TcpConnection Loop is null!";
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr,
                             const InetAddress &peerAddr)
                             : loop_(checkLoopNotNull(loop))
                             , name_(name)
                             , state_(kConnecting)
                             , reading_(true)
                             , socket_(new Socket(sockfd))
                             , channel_(new Channel(loop,sockfd))
                             , localAddr_(localAddr)
                             , peerAddr_(peerAddr)
                             , highWaterMark_(64*1024*1024)
{
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead,this,std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite,this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose,this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError,this));

    LOG << "TcpConnection::ctor:" << name.c_str() << "at fd=" << sockfd;
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
    LOG << "TcpConnection::dtor:" << name_.c_str() << "at fd=" << channel_->fd() << "state=" << state_;
}

void TcpConnection::send(const std::string &buf) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(buf.c_str(),buf.size());
        } else {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop,this,buf.c_str(),buf.size()));
        }
    }
}

// 发送数据 应用写的快 内核发送慢
// 需要将待发送的数据写入缓冲区中 设置水位回调
void TcpConnection::sendInLoop(const void *data, size_t len) {
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultErrno = false;

    // 之前调用该Connection的shutdown 不能进行发送
    if (state_ == kDisconnecting) {
        LOG << "disconnected,give up writing!";
        return;
    }
    // channel第一次写数据并且缓冲区中没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(),data,len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (reading_ == 0 && writeCompleteCallback_) {
                // 数据全部发送完成 就不再需要给channel设置epollout事件了
                loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));

            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG << "TcpConnection::sendInLoop";
                if (errno == EPIPE || errno == ECONNRESET) {
                    faultErrno = true;
                }
            }
        }
    }

    // 并没有将数据全部发送出去
    // 剩余的数据需要保存到缓冲区中 然后给channel
    // 注册epollout事件 poller发现tcp的发送缓冲区有空间 会通知相应的sock-channel 调用writeCallback回调方法
    // 调用TcpConnection::handleWrite方法将发送缓冲区中数据全部发送完成
    if (!faultErrno && remaining > 0) {
        // 目前发送缓冲区剩余的待发送数据长度
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_
            && oldLen < highWaterMark_
            && highWaterMark_) {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_,shared_from_this(),oldLen+remaining));
        }
        outputBuffer_.append((char*)data + nwrote,remaining);
        if (!channel_->isWriting()) {
            channel_->enableReading(); // 否则poller不会给channel通知epollout
        }
    }
}

void TcpConnection::shutdown() {
    if (state_ == kConnected) {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop,this));
    }
}

void TcpConnection::shutdownInLoop() {
    // 数据已经全部发送完成
    if (!channel_->isWriting()) {
        socket_->shutdownWrite(); // 关闭写端
    }
}

void TcpConnection::connectEstablished() {
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    // 新连接建立 执行回调
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
    if (state_ == kConnected) {
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime) {
    int saveError = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(),&saveError);
    if (n > 0) {
        // 有可读事件发送 调用用户传入的回调操作
        messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);
    } else if (n == 0) {
        handleClose();
    } else {
        errno = saveError;
        LOG << "TcpConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleWrite() {
    if (channel_->isWriting()) {
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(),&saveErrno);
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
                }
                if (state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            LOG << "TcpConnection::handleWrite";
        }
    } else {
        LOG << "TcpConnection fd=" << channel_->fd() << "is down,no more writing";
    }
}

void TcpConnection::handleClose() {
    LOG << "TcpConnectioon::handleClose fd=" << channel_->fd() << "state=" << state_;
    setState(kDisconnected);
    channel_->disableAll();

    // ???
    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); // 执行关闭连接的回调
    closeCallback_(connPtr); // 关闭连接的回调
}

void TcpConnection::handleError() {
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen) < 0) {
        err = errno;
    } else {
        err = optval;
    }
    LOG << "TcpConnection::handleError name:" << name_.c_str() << "SO_ERROR" << err;
}

