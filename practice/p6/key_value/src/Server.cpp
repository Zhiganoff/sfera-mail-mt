#include <Server.h>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>
#include <cstdlib>


using std::strerror;

Server::Server(uint32_t port) {
	this->sockfd = socket(AF_INET, SOCK_STREAM, 0); // 0 for auto
	if (this->sockfd == -1) {
		fprintf(stderr, "Socket not created: %s\n", strerror(errno));
	} else {
		fprintf(stderr, "Socket created on 0.0.0.0:%d\n", port);
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int reuseaddr = 1;

	int r = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (int *) &reuseaddr, sizeof(int));
	if (r == -1) {
		fprintf(stderr, "Reuse addr failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	r = bind (sockfd, (struct sockaddr *) &addr, sizeof(addr));
	if (r == -1) {
		fprintf(stderr, "Socket failed on bind: %s\n", strerror(errno));
	} else {
		fprintf(stderr, "Socket binded to 0.0.0.0:%d\n", port);
	}
	listen(sockfd, 2);

	// Now create Memory:
	try {
		memory.create();
	} catch (const char *str) {
		fprintf(stderr, "Memory create fatal failed: %s\n", str);
		fprintf(stderr, "Stopping server\n");
		exit(EXIT_FAILURE);
	}

	sem.init();

	if (!fork()) {
		fprintf(stderr, "[%d]: Defragmentator created!\n", getpid());

		memory.become_child();
		memory.attach(memory.get_id());

		while (1) {
			sem.ack();
			memory.defrag();
			sem.release();
			sleep(3);
		}
		exit(EXIT_SUCCESS);
	}

	if (!fork()) {
		fprintf(stderr, "[%d]: TTL-killer created\n", getpid());

		memory.become_child();
		memory.attach(memory.get_id());

		while(1) {
			sem.ack();
			memory.ttlkiller();
			sem.release();
			sleep(1);
		}
		exit(EXIT_SUCCESS);
	}
}

void Server::accept_connections() {
	struct sockaddr_in client;
	socklen_t client_len = sizeof(client);

	while (1) {
		int clientfd = accept(sockfd, (struct sockaddr *) &client, &client_len );
		if (clientfd != -1) {
			printf("Client connection started: fd=%d\n", clientfd);
			if (!fork()) {
				this->serve_client(clientfd);
				exit(EXIT_SUCCESS);
			} else {
				close(clientfd);
				printf("Looks like fork is done!\n");
			}
		} else if (errno != EINTR and errno != EAGAIN) { // some unknown error accured
			fprintf(stderr, "Error on accept: %s\n", strerror(errno));
		}
	}
}

void Server::serve_client(int clientfd) {
	close(sockfd); // closing server

	memory.become_child(); // after fork it should be child!
	Client client(clientfd, memory.get_id(), this->sem);
}

Server::~Server() {
	fprintf(stderr, "Server destructor called\n");

	int status; pid_t chld;
	while((chld = waitpid(-1, &status, 0)) != -1) {
		fprintf(stderr, "[%d]: Child %d gone with: %d\n", getpid(), chld, WEXITSTATUS(status));
	}

	memory.detach();
	sem.destroy();
}

