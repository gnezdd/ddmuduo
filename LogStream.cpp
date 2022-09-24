//
// Created by gnezd on 2022-09-18.
//

#include "LogStream.h"

#include <algorithm>

template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;

// 整型数据转换为字符串
// 设计一个查询表能够快速找到十进制int类型数字对应的char字符
// 从给定输入数据最地位开始到最高位依次取余并将得到的十进制数字存入结果字符串中
// 最后加入符号位字符 再最后对结果逆序即可
const char digits[] = "9876543210123456789";
const char *zero = digits + 9;

// 将数据转换为字符串并返回数据长度
template<typename T>
size_t convert(char buf[], T value) {
    T i = value;
    char *p = buf;
    
    do {
        int lsd = static_cast<int>(i % 10);
        i /= 10;
        *p++ = zero[lsd];
    } while (i != 0);

    if (value < 0) *p++ = '-';

    *p = '\0';
    std::reverse(buf,p);

    return p - buf;
}

// 转换为字符串并填入缓冲区中
template<typename T>
void LogStream::formatInteger(T v) {
    // 如果buffer容不下kMaxNumericSize个字符会直接被丢弃
    if (buffer_.avail() >= kMaxNumericSize) {
        size_t len = convert(buffer_.current(),v);
        buffer_.add(len);
    }
}

// bool类型的转换
LogStream& LogStream::operator<<(bool v) {
    buffer_.append(v ? "1" : "0",1);
    return *this;
}

// 整型数据的转换
LogStream &LogStream::operator<<(short v) {
    // 先转换为int 由int的<<重载存入缓冲区中
    *this << static_cast<int>(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned short v) {
    *this << static_cast<unsigned int>(v);
    return *this;
}

LogStream &LogStream::operator<<(int v) {
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned int v) {
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(long v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long long v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long long v) {
    formatInteger(v);
    return *this;
}

// 浮点数据类型的转换
LogStream &LogStream::operator<<(double v) {
    if (buffer_.avail() >= kMaxNumericSize) {
        int len = snprintf(buffer_.current(),kMaxNumericSize,"%.12g",v);
        buffer_.add(len);
    }
    return *this;
}

LogStream& LogStream::operator<<(long double v) {
    if (buffer_.avail() >= kMaxNumericSize) {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12Lg", v);
        buffer_.add(len);
    }
    return *this;
}

LogStream &LogStream::operator<<(float v) {
    *this << static_cast<double>(v);
    return *this;
}

// 字符类型的转换
LogStream &LogStream::operator<<(char v) {
    buffer_.append(&v,1);
    return *this;
}

LogStream &LogStream::operator<<(const char *str) {
    if (str) buffer_.append(str, strlen(str));
    else buffer_.append("(null)",6);
    return *this;
}

LogStream &LogStream::operator<<(const unsigned char *str) {
    return operator<<(reinterpret_cast<const char*>(str));
}

// 字符串类型的转换
LogStream &LogStream::operator<<(const std::string &v) {
     buffer_.append(v.c_str(),v.size());
     return *this;
}