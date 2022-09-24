//
// Created by gnezd on 2022-09-18.
//

#include "LogFile.h"

LogFile::LogFile(const std::string &basename, int flushEveryN)
    : basename_(basename)
    , flushEveryN_(flushEveryN)
    , count_(0)
    , mutex_() {
    file_.reset(new AppendFile(basename));
}

LogFile::~LogFile() {}

void LogFile::append(const char *logline, int len) {
    std::unique_lock<std::mutex> lock(mutex_);
    // 向文件写入就不需要加锁了
    append_unlocked(logline,len);
}

void LogFile::flush() {
    std::unique_lock<std::mutex> lock(mutex_);
    file_->flush();
}

void LogFile::append_unlocked(const char *logline, int len) {
    file_->append(logline,len);
    count_++;
    if (count_ >= flushEveryN_) {
        count_ = 0;
        file_->flush();
    }
}