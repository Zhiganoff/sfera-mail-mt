#include <Client.h>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <Message.h>
#include <Memory.h>
#include <iostream>


Client::Client(int clientfd, int shmid, Semaphore server_sem):
			  memory(), sockfd(clientfd), sem(server_sem) {
	fprintf(stderr, "Client connection created. Start processing. fd=%d\n", clientfd);
	memory.become_child();
	memory.attach(shmid);
	this->process();
}


void Client::process() {
	while (1) {
		Message msg = Message();

		int res = recv(sockfd, &msg.len, sizeof(msg.len), 0);// i wanna read 4 bytes to known length of msg
		if (res == -1) {
			fprintf(stderr, "Error while reading client msg: %s; droping connection\n", strerror(errno));
			this->drop();
			return;
		}
		if (msg.len < 1) {
			this->drop();
			return;
		}


		res = recv(sockfd, &msg.command, sizeof(msg.command), 0);
		if (res == -1) {
			fprintf(stderr, "Error while reading client msg: %s; droping connection\n", strerror(errno));
			this->drop();
			return;
		}

		if (msg.command == MESSAGE_CONNECT) {
			fprintf(stderr, "Connect!\n");
		} else if (msg.command == MESSAGE_DISCONNECT) {
			fprintf(stderr, "Disconnect\n");
			break;
		} else if (
			msg.command == MESSAGE_GET or
			msg.command == MESSAGE_SET or
			msg.command == MESSAGE_DELETE)
		{

			msg.resize_buf(msg.len);

			res = recv(sockfd, msg.buf, msg.len, 0);
			if (res == -1) {
				fprintf(stderr, "Error while reading client msg: %s; droping connection\n", strerror(errno));
				break;
			}

			if (not next(msg)) {
				break;
			}

		} else {
			fprintf(stderr, "Unknown command: %d\n", msg.command);
			break;
		}

	}
	fprintf(stderr, "Connection closed by server\n");
	this->drop();
}


void Client::drop() {
	fprintf(stderr, "Droping client connection for fd=%d\n", sockfd);
	memory.detach();
	shutdown(sockfd, 2);
	close(sockfd);
}

int Client::next(Message & msg) {
	size_t key_size = 0;
	char *cur_buf = msg.buf;
	switch (msg.command) {
		case MESSAGE_SET:
		{
			uint32_t ttl;
			memcpy(&ttl, cur_buf, sizeof(ttl)); // = *((uint32_t *)cur_buf);
			cur_buf += sizeof(ttl);

			while ((key_size < msg.len) && cur_buf[key_size]) {
				key_size++;
			}

			if (msg.len - sizeof(msg.command) == key_size) {
				return 0;
			}

			uint32_t val_size = msg.len - msg.command - sizeof(ttl) - sizeof(msg.len) - key_size;

			char key[key_size + 1] = {}, value[val_size + 1] = {};
			strncpy(key, cur_buf, key_size);
			strncpy(value, cur_buf + key_size + 1, val_size);

			fprintf(stderr, "Set: (%d) %s => %s\n", ttl, key, value);

			sem.ack();
			try {
				memory.set(ttl, std::string(key), std::string(value));
				sem.release();
				this->reply("ok");
				return 1;
			} catch(const std::string & s) { 
				sem.release();
				if (s == "too_big_value") {
					this->reply("failed");
					return 0;
				}
				fprintf(stderr, "%s\n", s.c_str());
				return 0;
			} catch (const char * msg) {
				fprintf(stderr, "%s\n", msg);
				sem.release();
				return 0;
			}

			break;
		}
		case MESSAGE_GET:
		{
			while ((key_size < msg.len) && cur_buf[key_size]) {
				key_size++;
			}

			char key[key_size + 1] = {};
			strncpy(key, cur_buf, key_size);

			fprintf(stderr, "Get: %s\n", key);

			sem.ack();
			try {
				std::string res = memory.get(std::string(key));
				sem.release();
				this->reply(res);
				return 1;
			} catch (std::string & s) {
				sem.release();
				if (s == "not_found") {
					this->reply(s);
					return 1;
				}
				fprintf(stderr, "%s\n", s.c_str());
				return 0;
			} catch (const char * msg) {
				fprintf(stderr, "%s\n", msg);
				sem.release();
				return 0;
			}

			break;
		}
		case MESSAGE_DELETE:
		{

			while ((key_size < msg.len) && cur_buf[key_size]) {
				key_size++;
			}

			char key[key_size + 1] = {};
			strncpy(key, cur_buf, key_size);

			fprintf(stderr, "Remove: %s\n", key);

			sem.ack();
			try {
				memory.remove(std::string(key));
				sem.release();
				this->reply("ok");
				return 1;
			} catch (const char * msg) {
				fprintf(stderr, "%s\n", msg);
				sem.release();
				return 0;
			}
			
			break;
		}
	}
	return 1;
}

void Client::reply(std::string answer) {
	uint32_t len = answer.size();
	char buf[len + sizeof(len)] = {};

	fprintf(stderr, "Response len = %u\n", len);

	memcpy(buf, &len, sizeof(len));
	memcpy(buf + sizeof(len), answer.c_str(), len);

	int r = send(sockfd, &buf, sizeof(buf), 0);

	if (r == -1) {
		fprintf(stderr, "Response sending failed due: %s\n", strerror(errno));
	} else {
		fprintf(stderr, "Sended %d/%lu\n", r, sizeof(buf));
	}
}
