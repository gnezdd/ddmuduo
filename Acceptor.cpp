//
// Created by gnezd on 2022-08-23.
//

#include "Acceptor.h"
#include "Logging.h"
#include "InetAddress.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <error.h>
#include <unistd.h>
#include <fcntl.h>

static int createNonblocking() {
    int sockfd = ::socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,0);
    if (sockfd < 0) {
        LOG << "listen socket create err" << errno;
    }
    return sockfd;
}

static int createIdleFd() {
    int idleFd = ::open("/dev/null",O_RDONLY | O_CLOEXEC);
    if (idleFd < 0) LOG << "idlfFd_ create err:" << errno;
    return idleFd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonblocking())
    , acceptChannel_(loop,acceptSocket_.fd())
    , listenning_(false)
    , idleFd_(createIdleFd()){
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr); // bind
    // TcpServer::start() Acceptor.listen 有新用户连接 需要执行一个回调
    // baseLoop=>acceptChannel_(listenfd)
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen() {
    listenning_ = true;
    acceptSocket_.listen(); // listen
    acceptChannel_.enableReading();
}

void Acceptor::handleRead() {
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0) {
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd,peerAddr);
        } else {
            ::close(connfd);
        }
    } else {
        LOG << "accept err:" << errno;
        // 文件描述符耗尽
        if (errno == EMFILE) {
            LOG << "sockfd reached limit!";
            ::close(idleFd_);
            idleFd_ = ::accept(acceptSocket_.fd(),NULL,NULL);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null",O_RDONLY | O_CLOEXEC);
        }
    }

}