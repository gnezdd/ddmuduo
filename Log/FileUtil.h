//
// Created by gnezd on 2022-09-19.
//

#ifndef DDMUDUO_FILEUTIL_H
#define DDMUDUO_FILEUTIL_H

#include "noncopyable.h"

#include <string>

// 含文件指针指向外部文件
class AppendFile : noncopyable {
public:
    explicit AppendFile(std::string filename);
    ~AppendFile();

    // 开放给外部的接口 写入日志文件的缓冲区中 也就是buffer_
    void append(const char *logline,const size_t len);
    // 将缓冲区中内容刷新到日志文件中
    void flush();

private:
    // 向文件写入内容
    size_t write(const char* logline, size_t len);
    // 日志文件的文件描述符
    FILE *fp_;
    // 缓冲区大小
    char buffer_[64 * 1024];
};

#endif //DDMUDUO_FILEUTIL_H
