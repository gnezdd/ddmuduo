//
// Created by gnezd on 2022-08-24.
//

#ifndef DDMUDUO_CALLBACKS_H
#define DDMUDUO_CALLBACKS_H

#include <memory>
#include <functional>

class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;

using MessageCallback = std::function<void(const TcpConnectionPtr&,
                                        Buffer*,
                                        Timestamp)>;

using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&,size_t)>;
using TimerCallback = std::function<void()>;

#endif //DDMUDUO_CALLBACKS_H
