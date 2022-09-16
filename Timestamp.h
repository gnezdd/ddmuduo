//
// Created by gnezd on 2022-08-20.
//

#ifndef DDMUDUO_TIMESTAMP_H
#define DDMUDUO_TIMESTAMP_H

#include <iostream>
#include <string>

// 时间类
class Timestamp {
public:
    // 构造函数
    Timestamp();
    explicit Timestamp(int64_t micreSecondsSinceEpoch);
    // 获取时间
    static Timestamp now();
    // 转化为字符串
    std::string toString() const;
private:
    int64_t microSecondsSinceEpoch_;
};

#endif //DDMUDUO_TIMESTAMP_H
