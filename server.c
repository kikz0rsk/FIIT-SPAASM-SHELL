#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "common.h"
#include "server.h"
#include "arguments.h"
#include "parser.h"

void* client_subroutine(void* client_fd);
void* accept_clients_routine(void* arguments);
char* build_prompt();
void execute_command(struct command* command, int clientFd);

int server(struct arguments* args, char* target, int port, bool unix_socket) {
	int* sockHandle = calloc(1, sizeof(int));
	struct sockaddr* addr;
	int sockAddrSize;

	if (unix_socket) {
		struct sockaddr_un* address = calloc(1, sizeof(struct sockaddr_un));
		*sockHandle = socket(AF_UNIX, SOCK_STREAM, 0);
		address->sun_family = AF_LOCAL;
		strcpy(address->sun_path, target);
		unlink(target);
		addr = (struct sockaddr*)address;
		sockAddrSize = sizeof(struct sockaddr_un);
	}
	else {
		struct sockaddr_in* address = calloc(1, sizeof(struct sockaddr_in));
		*sockHandle = socket(AF_INET, SOCK_STREAM, 0);
		address->sin_family = AF_INET;
		address->sin_port = htons(port);
		address->sin_addr.s_addr = INADDR_ANY;
		addr = (struct sockaddr*)address;
		sockAddrSize = sizeof(struct sockaddr_in);
	}

	if (*sockHandle == -1) {
		printf("Failed to create socket: %s (%d)\n", strerror(errno), errno);
		return 1;
	}

	if (bind(*sockHandle, addr, sockAddrSize) < 0) {
		perror("Bind error");
		return 1;
	}

	listen(*sockHandle, 10);
	pthread_t* acceptClientsThread = calloc(1, sizeof(pthread_t));

	pthread_create(acceptClientsThread, NULL, accept_clients_routine, sockHandle);

	return 0;
}

void* accept_clients_routine(void* arguments) {
	int sockHandle = *(int*)arguments;
	while (true) {
		struct sockaddr_in* caddr = calloc(1, sizeof(struct sockaddr_in));
		socklen_t len = sizeof(struct sockaddr_in);

		int* clientFd = calloc(1, sizeof(int));
		*clientFd = accept(sockHandle, (struct sockaddr*)caddr, &len);

		if (clientFd < 0) {
			perror("Client failed to connect: ");
			continue;
		}

		pthread_t* clientThread = calloc(1, sizeof(pthread_t));

		pthread_create(clientThread, NULL, client_subroutine, clientFd);
	}
}

void* client_subroutine(void* _clientFd) {
	int* clientFd = (int*)_clientFd;

	char buffer[1024] = { 0 };
	int bytesRead = 0;
	while (true)
	{
		char* prompt = build_prompt();
		int res = write(*clientFd, prompt, strlen(prompt) + 1);
		safe_free((void*)&prompt);

		if (res <= 0) {
			break;
		}

		bytesRead = read(*clientFd, &buffer, sizeof(buffer));

		if (bytesRead <= 0) {
			break;
		}

		buffer[bytesRead] = 0;

		struct command* command = parse_input(buffer);
		while (command != NULL) {
			execute_command(command, *clientFd);
			struct command* nextCommand = command->nextCommand;
			free_command(command);
			command = nextCommand;
		}
	}

	free(clientFd);
}

void execute_command(struct command* command, int clientFd) {
	if (strcmp(command->arguments[0], "echo") == 0) {
		char allArguments[1024] = { 0 };

		if (command->arguments > 0) {
			strcat(allArguments, command->arguments[1]);
			for (int i = 2; i < command->numArguments; i++) {
				char* tmp;
				asprintf(&tmp, " %s", command->arguments[i]);
				strcat(allArguments, tmp);
				safe_free((void**)&tmp);
			}
		}

		strcat(allArguments, "\n");
		write(clientFd, allArguments, strlen(allArguments));
		return;
	}
	else if (strcmp(command->arguments[0], "quit") == 0) {
		close(clientFd);
		return;
	}
	else if (strcmp(command->arguments[0], "halt") == 0) {
		close(clientFd);
		exit(0);
	}

	pid_t pid = fork();
	if (pid == 0) {
		// New process - child
		dup2(clientFd, 0);
		dup2(clientFd, 1);
		dup2(clientFd, 2);

		if (execvp(command->arguments[0], command->arguments) == -1) {
			perror("exec");
		}
		exit(-1);
	}
	else if (pid < 0) {
		perror("fork");
		return;
	}

	// Parent process
	int status;
	do {
		waitpid(pid, &status, WUNTRACED);
	} while (!WIFEXITED(status) && !WIFSIGNALED(status));
}

char* build_prompt() {
	char hostname[50];
	gethostname(hostname, sizeof(hostname));

	time_t tm = time(NULL);
	struct tm localTm = *localtime(&tm);

	char* prompt;
	asprintf(&prompt, "%02d:%02d %s@%s# ", localTm.tm_hour, localTm.tm_min, cuserid(NULL), hostname);
	return prompt;
}