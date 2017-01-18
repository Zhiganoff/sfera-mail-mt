#pragma once
#include <iostream>
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <Message.h>
#include <Memory.h>
#include <Semaphore.h>
#include <string.h>

class Client {
	Memory memory;
	int sockfd;
    Semaphore sem;
public:
	Client(int clientfd, int shmid, Semaphore server_sem);
	void process();
	void drop();
	int next(Message & msg);
	void reply(std::string);
};
