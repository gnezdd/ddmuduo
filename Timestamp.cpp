//
// Created by gnezd on 2022-08-20.
//

#include "Timestamp.h"

#include <sys/time.h>
#include <stdio.h>

// 构造函数
Timestamp::Timestamp() : microSecondsSinceEpoch_(0) {}

Timestamp::Timestamp(int64_t micreSecondsSinceEpoch)
    : microSecondsSinceEpoch_(micreSecondsSinceEpoch)
    {}

// 获取时间
Timestamp Timestamp::now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * Timestamp::kMicroSecondsPerSecond + tv.tv_usec);
}

// 转化为字符串
std::string Timestamp::toString() const {
    char buf[128] = {0};
    tm *tm_time = localtime(&microSecondsSinceEpoch_);
    snprintf(buf,128,"%4d-%02d-%02d %02d:%02d:%02d",
             tm_time->tm_year + 1900,
             tm_time->tm_mon + 1,
             tm_time->tm_mday,
             tm_time->tm_hour,
             tm_time->tm_min,
             tm_time->tm_sec);
    return buf;
}


/*#include <iostream>
int main(){
    std::cout << Timestamp::now().toString() << std::endl;
    return 0;
}*/
