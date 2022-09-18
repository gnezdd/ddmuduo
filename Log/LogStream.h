//
// Created by gnezd on 2022-09-18.
//

#ifndef DDMUDUO_LOGSTREAM_H
#define DDMUDUO_LOGSTREAM_H

#include <string>
#include <string.h>

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

// 日志流的缓冲区 addcode noncopyable
// 非类型参数的模板类 通过传入SIZE表示缓冲区大小
template <int SIZE>
class FixedBuffer {
public:
    FixedBuffer() : cur_(data_) {}
    ~FixedBuffer();

    // 获取缓冲区中剩余的空闲空间
    int avail() const {return static_cast<int>(end() - cur_);}

    // 获取缓冲区地址
    const char* data() const {return data_;}

    // 获取缓冲区写入了多少字节
    int length() const {return cur_ - data_;}

    // 获取缓冲区的可写位置
    char* current() {return cur_;}

    // 移动缓冲区可写位置指针
    void add(size_t len) {cur_ += len;}

    // 添加内容到缓冲区中
    void append(const char* buf, size_t len) {
        // 如果缓冲区剩余空间>len则存放
        if (avail() > static_cast<int>(len)) {
            memcpy(cur_,buf,len);
            // 移动cur_指针
            add(len);
        }
    }

    // 重置缓冲区可写位置指针
    void reset() {cur_ = data_;}

    // 清空缓冲区内容
    void bzero() { memset(data_,0,sizeof data_);}

private:
    // 获取缓冲区的末尾地址
    const char *end() const {return data_ + sizeof data_;}

    char data_[SIZE]; // 缓冲区的存储空间
    char *cur_; // 缓冲区当前可写位置
};

// 负责将日志内容存放到缓冲区中 包含各种类型数据
// 需要向将数据转换为字符串类型再存放入缓冲区 对类型重置<<操作
// 通过重载实现日志流
// 最终都是调用append函数
// append调用LogStream中的buffer的append将日志内容存放进缓冲区中
// addcode noncopyable
class LogStream {
public:
    // 给缓冲区类型重命名
    using Buffer = FixedBuffer<kSmallBuffer>;

    // 将内容添加到缓冲区中
    void append(const char* data,int len) {buffer_.append(data,len);}
    // 获取日志流对应的缓冲区
    const Buffer& buffer() const {return buffer_;}
    // 重置缓冲区
    void resetBuffer() {buffer_.reset();}

    // 重载<<

    // 将bool类型数据存入缓冲区中
    LogStream& operator<<(bool v);

    // 整型数据的字符串转换保存到缓冲区中
    // 内部均调用formatInterger
    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    // 浮点数据类型的转换
    LogStream& operator<<(double);
    LogStream& operator<<(long double);
    LogStream& operator<<(float);

    // 字符类型的转换
    LogStream& operator<<(char);
    LogStream& operator<<(const char*);
    LogStream& operator<<(const unsigned char *);

    // 字符串类型的转换
    LogStream& operator<<(const std::string&);

private:
    
    // 整型数据转换字符串的模板函数
    template<typename T>
    void formatInteger(T);

    // 日志流的缓冲区
    Buffer buffer_;

    // 最大的有效数字的位数
    static const int kMaxNumericSize = 32;
};



#endif //DDMUDUO_LOGSTREAM_H
