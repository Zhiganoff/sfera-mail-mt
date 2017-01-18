#pragma once

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <cstring>
#include <cstdio>
#include <errno.h>

class Semaphore {
    int semid;
public:
    Semaphore();
    int init();
    int ack();
    int release();
    void destroy();
};