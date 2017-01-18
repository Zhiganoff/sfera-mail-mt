#include <Server.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>


pid_t server_pid; Server *server = nullptr;
void Server::sigHandler(int signum) {
	fprintf(stderr, "[%d] Catch: %d grp=%d\n", getpid(), signum, getpgrp());

	if (signum == 2) {
		kill(-server_pid, 15); // kill group of children
	} else if (signum == 17) {
		int status = 0;
		waitpid(-1, &status, 0);
		fprintf(stderr, "Chld gone with status: %d\n", WEXITSTATUS(status));
	} else {
		if (getpid() == getpgrp()) { // if I'm really server
			delete server;
		}
		exit(EXIT_SUCCESS);
	}
}


int main(int argc, char const *argv[])
{

	struct sigaction sa;
	sa.sa_handler = Server::sigHandler;
	sa.sa_flags = 0;

	sigaction(SIGTERM, &sa, 0);
	sigaction(SIGINT, &sa, 0);
	sigaction(SIGCHLD, &sa, 0);

	sigset_t sigset;
	sigfillset(&sigset);
	sigdelset(&sigset, SIGTERM);
	sigdelset(&sigset, SIGINT);
	sigdelset(&sigset, SIGCHLD);
	sigprocmask(SIG_SETMASK, &sigset, 0);

	fprintf(stderr, "My pid: %d\n", getpid());
	fprintf(stderr, "PGid: %d\n", getpgrp());

	server_pid = fork();
	if (!server_pid) {

		setpgid(0,0);


		server = new Server(8090);
		server->accept_connections();
		return 0;
	}

	int status; pid_t chld;
	while((chld = waitpid(server_pid, &status, 0)) != -1) {
		fprintf(stderr, "Child %d gone with: %d\n", chld, WEXITSTATUS(status));
	}

	fprintf(stderr, "All children is gone\n");
	return 0;
}