//
// Created by gnezd on 2022-09-18.
//

#ifndef DDMUDUO_COUNTDOWNLATCH_H
#define DDMUDUO_COUNTDOWNLATCH_H

#include "noncopyable.h"

#include <mutex>
#include <condition_variable>

// 确保Thread中传进去的func启动后外层的start才返回
class CountDownLatch : noncopyable {
public:
    explicit CountDownLatch(int count);
    void wait();
    void countDown();

private:
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    int count_;
};

#endif //DDMUDUO_COUNTDOWNLATCH_H
