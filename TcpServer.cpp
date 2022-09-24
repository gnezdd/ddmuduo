//
// Created by gnezd on 2022-08-20.
//

#include "TcpServer.h"
#include "Logging.h"
#include "TcpConnection.h"

#include <string.h>

static EventLoop* checkLoopNotNull(EventLoop *loop) {
    if (loop == nullptr) {
        LOG << "mainLoop id null!";
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option)
    : loop_(checkLoopNotNull(loop))
    , ipPort_(listenAddr.toIpPort())
    , name_(nameArg)
    , acceptor_(new Acceptor(loop,listenAddr,option == kReusePort))
    , threadPool_(new EventLoopThreadPool(loop,name_))
    , connectionCallback_()
    , messageCallback_()
    , nextConnId_(1)
    , started_(0) {
    // 当有新用户连接时 会执行回调
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,
                                                  std::placeholders::_1,std::placeholders::_2));
}

TcpServer::~TcpServer() {
    for (auto &item : connections_) {
        // 该局部shared_ptr智能指针对象 出右括号可以自动释放new出来的TcpConnection对象资源
        TcpConnectionPtr conn(item.second);
        item.second.reset();

        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
    }
}

void TcpServer::start() {
    // 防止一个TcpServer被启动多次
    if (started_++ == 0) {
        threadPool_->start(threadInitCallback_); // 启动底层线程池
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));
    }
}

void TcpServer::setThreadNum(int numThreads) {
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
    // 选择一个subLoop管理channel
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf,sizeof buf, "-%s#%d",ipPort_.c_str(),nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    // 通过sockfd获取其绑定的本机ip地址和端口
    sockaddr_in local;
    ::bzero(&local,sizeof local);
    socklen_t addrlen = sizeof local;
    if (::getsockname(sockfd,(sockaddr*)&local,&addrlen) < 0) {
        LOG << "sockets::getLocalAddr";
    }

    InetAddress localAddr(local);

    // 根据连接成功的sockfd 创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(ioLoop,connName,sockfd,localAddr,peerAddr));

    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    conn->setCloseCallback(std::bind(&TcpServer::removeConnection,this,std::placeholders::_1));
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop,this,conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {

    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
}