//
// Created by gnezd on 2022-09-18.
//

#include "CountDownLatch.h"

CountDownLatch::CountDownLatch(int count)
    : mutex_()
    , cond_()
    , count_(count) {

}

void CountDownLatch::wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (count_ > 0) cond_.wait(lock);
}

void CountDownLatch::countDown() {
    std::unique_lock<std::mutex> lock(mutex_);
    --count_;
    if (count_ == 0) cond_.notify_all();
}