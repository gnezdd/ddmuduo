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

    int64_t microSecondsSinceEpoch() const {return microSecondsSinceEpoch_;}

     static const int kMicroSecondsPerSecond = 1000 * 1000;
private:
    int64_t microSecondsSinceEpoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline Timestamp addTime(Timestamp timestamp,double seconds) {
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch()+delta);
}

#endif //DDMUDUO_TIMESTAMP_H
