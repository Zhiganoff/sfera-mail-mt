#include <sys/socket.h>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <Memory.h>


int main(int argc, char const *argv[])
{
	// int serv = socket(AF_INET, SOCK_STREAM, 0);
	// fprintf(stderr, "serv=%d\n", serv);

	// struct sockaddr_in server;
	// server.sin_family = AF_INET;
	// server.sin_port = htons(8090);

	// inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);
	// socklen_t len = sizeof(server);

	// int r = connect(serv, (struct sockaddr *) &server, len);
	// fprintf(stderr, "%s\n", strerror(errno));


	// char *buf = "0";// = {123, 0};
	// r = sendto(serv, buf, sizeof(buf), 0, (struct sockaddr *) &server, len);
	// fprintf(stderr, "%s\n", strerror(errno));

	// shutdown(serv,2);
	// close(serv);

	Memory mem(0);

if (0) {
	try {
		fprintf(stderr, "Set:\n");
		mem.set(100, "ololo", "val1");

		fprintf(stderr, "\nGet:\n");
		mem.get("ololo");



		fprintf(stderr, "Set:\n");
		mem.set(100, "ololo", "val2");

		fprintf(stderr, "\nGet:\n");
		mem.get("ololo");



		fprintf(stderr, "Set:\n");
		mem.set(100, "ololo", "value3");

		fprintf(stderr, "\nGet:\n");
		mem.get("ololo");



		fprintf(stderr, "Set:\n");
		mem.set(100, "ololo", "v4");

		fprintf(stderr, "\nGet:\n");
		mem.get("ololo");
	} catch (const char * msg) {
		fprintf(stderr, "ERROR: %s\n", msg);
	};
}

	try {
		fprintf(stderr, "\nGet:\n");
		mem.get("kek");
	} catch (const char * msg) {
		fprintf(stderr, "ERROR: %s\n", msg);
	}

	try {

		fprintf(stderr, "\nSet:\n");
		mem.set(0, "fuck", "fuck");

		sleep(1);

		fprintf(stderr, "\nGet:\n");
		mem.get("fuck");

	} catch(const char * msg) {
		fprintf(stderr, "ERROR: %s\n", msg);
	}

	try {

		fprintf(stderr, "\nSet:\n");
		mem.set(100, "testdel", "blabla");

		fprintf(stderr, "\nGet:\n");
		mem.get("testdel");

		fprintf(stderr, "\nDelete:\n");
		mem.remove("testdel");

		fprintf(stderr, "\nGet:\n");
		mem.get("testdel");

	} catch(const char * msg) {
		fprintf(stderr, "ERROR: %s\n", msg);
	}


	return 0;
}
