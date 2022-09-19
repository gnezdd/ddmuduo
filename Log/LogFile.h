//
// Created by gnezd on 2022-09-18.
//

#ifndef DDMUDUO_LOGFILE_H
#define DDMUDUO_LOGFILE_H

#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "FileUtil.h"

// unique_ptr包装AppendFile类实例file_
// 后端线程写出时调用LogFile的append函数会通过该实例调用AppendFile的append函数将后端缓冲区中内容全部写入到日志文件中
class LogFile : noncopyable {
public:
    LogFile(const std::string &basename,int flushEveryN = 1024);
    ~LogFile();

    void append(const char *logline,int len);
    void flush();

private:
    void append_unlocked(const char* logline, int len);

    const std::string basename_;
    // 每被append了多少次就flush向文件中写入 但是文件也是有缓冲区的
    const int flushEveryN_;

    int count_; // 记录append了多少次
    std::mutex mutex_;
    std::unique_ptr<AppendFile> file_;
};

#endif //DDMUDUO_LOGFILE_H
