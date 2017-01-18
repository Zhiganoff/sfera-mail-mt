#pragma once
#include <iostream>
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include <Client.h>
#include <Message.h>
#include <Memory.h>
#include <Semaphore.h>

class Server {
	int sockfd;
	Memory memory;
    Semaphore sem;
public:
	Server(uint32_t port);
	void accept_connections();
	void serve_client(int clientfd);
	void drop(int clientfd);
	static void sigHandler(int signum);
	~Server();
};
