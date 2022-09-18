//
// Created by gnezd on 2022-08-21.
//

#ifndef DDMUDUO_CURRENTTHREAD_H
#define DDMUDUO_CURRENTTHREAD_H

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread {
    extern __thread int t_cachedTid;

    void cacheTid();

    inline int tid() {
        if (__builtin_expect(t_cachedTid ==0, 0)) {
            cacheTid();
        }
        return t_cachedTid;
    }
}

#endif //DDMUDUO_CURRENTTHREAD_H
