#include "Semaphore.h"

Semaphore::Semaphore() {
    key_t key = ftok("/dev/sda1", 'R');
    this->semid = semget(key, 1, 0644 | IPC_CREAT);
}

int Semaphore::init() {
    semctl(this->semid, 0, SETVAL, 0);
    return this->release();
}

int Semaphore::ack() {
    struct sembuf mybuf;
    mybuf.sem_op = -1;
    mybuf.sem_flg = 0;
    mybuf.sem_num = 0;
    return semop(semid, &mybuf, 1) < 0;
}

int Semaphore::release() {
    struct sembuf mybuf;
    mybuf.sem_op = 1;
    mybuf.sem_flg = 0;
    mybuf.sem_num = 0;
    return semop(semid, &mybuf, 1);
}

void Semaphore::destroy() {
    if (semctl(this->semid, 0, IPC_RMID, 0) == -1) {
        fprintf(stderr, "Semaphore destruction failed: %s\n", strerror(errno));
    }
}
