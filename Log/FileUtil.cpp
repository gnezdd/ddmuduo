//
// Created by gnezd on 2022-09-19.
//

#include "FileUtil.h"
#include "iostream"

AppendFile::AppendFile(std::string filename)
    : fp_(fopen(filename.c_str(),"ae")) {
    // 将buffer_设置为文件的缓冲区
    ::setbuffer(fp_,buffer_,sizeof buffer_);
}

AppendFile::~AppendFile() {
    fclose(fp_);
}

void AppendFile::append(const char *logline, const size_t len) {
    size_t n = this->write(logline,len);
    size_t remain = len - n;
    // 如果还没写完就循环写入
    while (remain > 0) {
        size_t x = this->write(logline + n, remain);
        if (x == 0) {
            int err = ferror(fp_);
            if (err) fprintf(stderr, "AppendFile::append() failed !\n");
            break;
        }
        n += x;
        remain = len - n;
    }
}

void AppendFile::flush() {
    // 强迫将buffer_缓冲区中的数据写回日志文件中
    int res = fflush(fp_);

}

size_t AppendFile::write(const char *logline, size_t len) {
    // 外部会有锁 这里可以使用无锁版本
    // 如AppendFile的LogFile中本身会加锁
    return fwrite_unlocked(logline,1,len,fp_);
}