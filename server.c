#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#include "common.h"
#include "server.h"
#include "arguments.h"
#include "parser.h"
#include "kshell.h"


void* client_subroutine(void* client_fd);
void* accept_clients_routine(void* arguments);
char* build_prompt();
void execute_command(struct command* command, int clientFd);
int add_connection(struct connection* con);
void remove_connection(struct connection* con);

struct connection* connections[20] = { 0 };
pthread_mutex_t connectionsLock;

int server(struct arguments* args, char* target, int port, bool unix_socket, bool daemon) {
	pthread_mutex_init(&connectionsLock, NULL);

	int* sockHandle = calloc(2, sizeof(int));
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

		if (target == NULL) {
			address->sin_addr.s_addr = INADDR_ANY;
		}
		else {
			inet_pton(AF_INET, target, &address->sin_addr.s_addr);
		}

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

	*(sockHandle + 1) = unix_socket;
	if (daemon) {
		accept_clients_routine((void*)sockHandle);
	}
	else {
		pthread_t* acceptClientsThread = calloc(1, sizeof(pthread_t));
		pthread_create(acceptClientsThread, NULL, accept_clients_routine, sockHandle);
	}
	
	return 0;
}

void* accept_clients_routine(void* arguments) {
	int sockHandle = *(int*)arguments;
	int unixSocket = *((int*)arguments + 1);
	while (true) {
		struct sockaddr* caddr = (struct sockaddr*)calloc(1, sizeof(struct sockaddr_un));
		socklen_t len = sizeof(struct sockaddr_un);

		int clientFd = accept(sockHandle, caddr, &len);
		if (clientFd < 0) {
			perror("Client failed to connect: ");
			continue;
		}

		pthread_t* clientThread = calloc(1, sizeof(pthread_t));

		struct connection* conn = calloc(1, sizeof(struct connection));
		conn->address = caddr;
		conn->addressLen = len;
		conn->thread = clientThread;
		conn->clientFd = clientFd;

		int index = add_connection(conn);
		if (index == -1) {
			char msg[] = "Maximum number of clients reached";
			write(clientFd, msg, sizeof(msg));
			close(clientFd);

			free(caddr);
			free(conn);
			free(clientThread);
			continue;
		}

		pthread_create(clientThread, NULL, client_subroutine, conn);
	}
}

void* client_subroutine(void* _connection) {
	struct connection* connection = (struct connection*)_connection;
	int clientFd = connection->clientFd;

	char buffer[2048] = { 0 };
	int bytesRead = 0;
	while (true)
	{
		char* prompt = build_prompt();
		int res = write(clientFd, prompt, strlen(prompt) + 1);
		safe_free((void**)&prompt);

		if (res <= 0) {
			break;
		}

		bytesRead = read(clientFd, &buffer, sizeof(buffer));

		if (bytesRead <= 0) {
			break;
		}

		buffer[bytesRead] = 0;

		struct command* command = parse_input(buffer);
		while (command != NULL) {
			execute_command(command, clientFd);
			struct command* nextCommand = command->nextCommand;
			free_command(command);
			command = nextCommand;
		}
	}

	close(clientFd);
	remove_connection(connection);
	free(connection->address);
	free(connection->thread);
	free(connection);
}

void execute_command(struct command* command, int clientFd) {
	if (strcmp(command->arguments[0], "quit") == 0) {
		close(clientFd);
		return;
	}
	else if (strcmp(command->arguments[0], "halt") == 0) {
		close(clientFd);
		exit(0);
	}
	else if (strcmp(command->arguments[0], "help") == 0) {
		char* help = get_help();
		write(clientFd, help, strlen(help) + 1);
		return;
	}
	else if (strcmp(command->arguments[0], "stat") == 0) {
		pthread_mutex_lock(&connectionsLock);
		char* line;
		for (int i = 0; i < sizeof(connections) / sizeof(struct connection*); i++) {
			if (connections[i] == NULL) {
				continue;
			}

			char string[108] = { 0 };
			if (connections[i]->addressLen == sizeof(struct sockaddr_in)) {
				// internet socket
				struct sockaddr_in* sock = (struct sockaddr_in*)connections[i]->address;
				inet_ntop(AF_INET, &sock->sin_addr.s_addr, string, sizeof(string));
			}
			else {
				// unix socket
				struct sockaddr_un* sock = (struct sockaddr_un*)connections[i]->address;
				strcpy(string, sock->sun_path);
			}

			asprintf(&line, "%d: %s\n", i, string);
			write(clientFd, line, strlen(line) + 1);
		}
		pthread_mutex_unlock(&connectionsLock);
		return;
	}

	pid_t pid = fork();
	if (pid == 0) {
		// Child

		if (command->pipeRedirect != NULL) {
			int pipes[2];
			pipe(pipes);

			pid_t pid = fork();
			if (pid < 0) {
				perror("pipe fork");
				exit(-1);
			}

			if (pid == 0) {
				// Child (command after pipe sign)
				// first pipe for reading
				dup2(pipes[0], STDIN_FILENO);
				close(pipes[1]);
				dup2(clientFd, STDOUT_FILENO);
				dup2(clientFd, STDERR_FILENO);
				if (execvp(command->pipeRedirect->arguments[0], command->pipeRedirect->arguments) == -1) {
					perror("exec");
				}
				exit(-1);
			}

			// Parent
			// second pipe for writing
			dup2(pipes[1], STDOUT_FILENO);
			close(pipes[0]);
			dup2(clientFd, STDERR_FILENO);
			dup2(clientFd, STDIN_FILENO);
			if (execvp(command->arguments[0], command->arguments) == -1) {
				perror("exec");
			}
			exit(-1);
		}

		int outputRedirectFd = clientFd;
		int inputRedirectFd = clientFd;

		if (command->outputRedirect != NULL) {
			outputRedirectFd = open(command->outputRedirect, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (outputRedirectFd < 0) {
				perror("output redirect open");
				return;
			}
		}

		if (dup2(outputRedirectFd, STDOUT_FILENO) < 0) {
			perror("dup output");
		}

		if (dup2(outputRedirectFd, STDERR_FILENO) < 0) {
			perror("dup output");
		}

		if (command->inputRedirect != NULL) {
			inputRedirectFd = open(command->inputRedirect, O_RDONLY);
			if (inputRedirectFd < 0) {
				perror("input redirect open");
				return;
			}
		}

		if (dup2(inputRedirectFd, STDIN_FILENO) < 0) {
			perror("dup input");
		}

		close(inputRedirectFd);
		close(outputRedirectFd);

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

int add_connection(struct connection* con) {
	pthread_mutex_lock(&connectionsLock);
	int index = -1;
	for (int i = 0; i < sizeof(connections) / sizeof(struct connection*); i++) {
		if (connections[i] == 0) {
			connections[i] = con;
			index = i;
			break;
		}
	}
	pthread_mutex_unlock(&connectionsLock);
	return index;
}

void remove_connection(struct connection* con) {
	pthread_mutex_lock(&connectionsLock);
	for (int i = 0; i < sizeof(connections) / sizeof(struct connection*); i++) {
		if (connections[i] == con) {
			connections[i] = NULL;
			break;
		}
	}
	pthread_mutex_unlock(&connectionsLock);
}