#include <Server.h>


int main(int argc, char const *argv[])
{
	int serv = socket(AF_INET, SOCK_STREAM, 0);
	fprintf(stderr, "serv=%d\n", serv);

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(8090);

	inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);
	socklen_t len = sizeof(server);

	int r = connect(serv, (struct sockaddr *) &server, len);
	fprintf(stderr, "%s\n", strerror(errno));

	enum {
		MESSAGE_CONNECT,
		MESSAGE_DISCONNECT,

		MESSAGE_GET,
		MESSAGE_SET,
		MESSAGE_DELETE
	};

	char buf[] = {1, MESSAGE_CONNECT};
	r = sendto(serv, buf, sizeof(buf), 0, (struct sockaddr *) &server, len);
	fprintf(stderr, "%s\n", strerror(errno));

	shutdown(serv,2);
	close(serv);

	return 0;
}