//
// Created by gnezd on 2022-08-24.
//

#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

// 从fd上读取数据 Poller工作在LT模式
// Buffer缓冲区是有大小的 但是从fd上读数据时不知道tcp数据最终的大小
ssize_t Buffer::readFd(int fd, int *saveErrno) {
    char extrabuf[65536] = {0}; // 栈上的内存空间
    struct iovec vec[2];
    const size_t writeable = writeableBytes(); // Buffe剩余的可写空间大小
    vec[0].iov_base = begin() + writeIndex_;
    vec[0].iov_len = writeable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writeable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd,vec,iovcnt);
    if (n < 0) {
        *saveErrno = errno;
    } else if (n <= writeable) {
        writeIndex_ += n;
    } else {
        writeIndex_ = buffer_.size();
        append(extrabuf,n-writeable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno) {
    ssize_t n = ::write(fd,peek(),readableBytes());
    if (n < 0) {
        *saveErrno = errno;
    }
    return n;
}