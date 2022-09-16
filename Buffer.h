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
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writeIndex_(kCheapPrepend)
    {}

    size_t readableBytes() const {
        return writeIndex_ - readerIndex_;
    }

    size_t writeableBytes() {
        return buffer_.size() - writeIndex_;
    }

    size_t prependalbeBytes() {
        return readerIndex_;
    }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const {
        return begin() + readerIndex_;
    }

    void retrieve(size_t len) {
        if (len < readableBytes()) {
            readerIndex_ += len; // 应用只读取了可读缓冲区中数据的一部分len
        } else {
            retrieveAll();
        }
    }

    void retrieveAll() {
        readerIndex_ = writeIndex_ = kCheapPrepend;
    }

    // 将onMessage函数上报的Buffer数据转成string类型的数据返回
    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes()); // 应用可读取数据的长度
    }

    std::string retrieveAsString(size_t len) {
        std::string result(peek(),len);
        retrieve(len);
        return result;
    }

    void ensureWriteableBytes(size_t len) {
        if (writeableBytes() < len) {
            makeSpace(len);
        }
    }

    // 将[data,date+len]内存上的数据添加到write缓冲区中
    void append(const char* data,size_t len) {
        ensureWriteableBytes(len);
        std::copy(data,data+len,beginWrite());
        writeIndex_ += len;
    }

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
    char* begin() {
        return &(*buffer_.begin()); // 数组的起始地址
    }

    const char* begin() const {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len) {
        if (writeableBytes() + prependalbeBytes() < len + kCheapPrepend) {
            buffer_.resize(writeIndex_ + len);
        } else {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                      begin() + writeIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writeIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writeIndex_;
};


#endif //DDMUDUO_BUFFER_H
