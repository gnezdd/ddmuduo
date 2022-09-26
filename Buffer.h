//
// Created by gnezd on 2022-08-24.
//

#ifndef DDMUDUO_BUFFER_H
#define DDMUDUO_BUFFER_H

#include <vector>
#include <string>
#include <algorithm>

// 网络库底层的缓冲器类型定义
class Buffer {
public:
    static const size_t kCheapPrepend = 8; // 占用缓冲区的前8个字节 用于表示缓冲区的大小
    static const size_t kInitialSize = 1024; // 缓冲区的初始化大小

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writeIndex_(kCheapPrepend)
    {}

    // 缓冲区中有多少字节可读
    size_t readableBytes() const {
        return writeIndex_ - readerIndex_;
    }

    // 缓冲区中还有多少字节可写
    size_t writeableBytes() {
        return buffer_.size() - writeIndex_;
    }

    // 获取缓冲区当前可读指针位置
    size_t prependalbeBytes() {
        return readerIndex_;
    }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const {
        return begin() + readerIndex_;
    }

    // 设置缓冲区中的readerIndex_指针位置
    void retrieve(size_t len) {
        if (len < readableBytes()) {
            // 应用只读取了可读缓冲区中数据的一部分len
            readerIndex_ += len;
        } else {
            // len = readableBytes 应用程序将缓冲区中数据都读完了
            // 那么需要将缓冲区的读写指针都移动到开头
            retrieveAll();
        }
    }

    // 将缓冲区的读写指针都复位
    void retrieveAll() {
        readerIndex_ = writeIndex_ = kCheapPrepend;
    }

    // 将onMessage函数上报的Buffer数据转成string类型的数据返回
    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes()); // 应用可读取数据的长度
    }

    // 将要读的内容转换为字符串
    std::string retrieveAsString(size_t len) {
        std::string result(peek(),len);
        retrieve(len);
        return result;
    }

    // 如果缓冲区不够要写入的数据则扩展
    void ensureWriteableBytes(size_t len) {
        if (writeableBytes() < len) {
            makeSpace(len);
        }
    }

    // 将[data,date+len]内存上的数据添加到write缓冲区中
    void append(const char* data,size_t len) {
        // 确保有足够的数据可以写入 如果没有则扩展缓冲区
        ensureWriteableBytes(len);
        // 将要写入的内容加入到缓冲区中
        std::copy(data,data+len,beginWrite());
        // 移动写指针
        writeIndex_ += len;
    }

    // 返回开始写位置的地址
    char* beginWrite() {
        return begin() + writeIndex_;
    }

    const char* beginWrite() const {
        return begin() + writeIndex_;
    }

    // 从fd上读数据
    ssize_t readFd(int fd,int* saveErrno);
    // 通过fd发送数据
    ssize_t writeFd(int fd,int* saveErrno);

private:

    // 数组的起始地址
    char* begin() {
        return &(*buffer_.begin());
    }

    const char* begin() const {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len) {
        // 如果可读指针前面空间和可写空间不够要存放的数据则扩展缓冲区空间
        if (writeableBytes() + prependalbeBytes() < len + kCheapPrepend) {
            buffer_.resize(writeIndex_ + len);
        } else {
            // 有多少个字节需要读写
            size_t readable = readableBytes();
            // 将可读写的内容移动到begin()+kCheapPrepend后
            std::copy(begin() + readerIndex_,
                      begin() + writeIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writeIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    // 不使用指针是为了防止迭代器失效
    size_t readerIndex_;
    size_t writeIndex_;
};


#endif //DDMUDUO_BUFFER_H
