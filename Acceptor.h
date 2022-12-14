//
// Created by gnezd on 2022-08-23.
//

#ifndef DDMUDUO_ACCEPTOR_H
#define DDMUDUO_ACCEPTOR_H

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

class Acceptor : noncopyable {
public:
    using NewConnectionCallback = std::function<void(int sockfd,const InetAddress&)>;
    Acceptor(EventLoop *loop,const InetAddress &listenAddr,bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &cb) {
        newConnectionCallback_ = cb;
    }

    bool listenning() const {return listenning_;}
    void listen();
private:
    void handleRead();
    EventLoop *loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback  newConnectionCallback_;
    bool listenning_;
    int idleFd_; // 空闲的文件描述符 用于避免文件描述符耗尽
};

#endif //DDMUDUO_ACCEPTOR_H
