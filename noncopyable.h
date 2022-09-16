//
// Created by gnezd on 2022-08-20.
//

#ifndef DDMUDUO_NONCOPYABLE_H
#define DDMUDUO_NONCOPYABLE_H

// 被继承以后 派生类对象可以正常构造和析构
// 但是无法进行拷贝构造和赋值
class noncopyable {
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

#endif //DDMUDUO_NONCOPYABLE_H
