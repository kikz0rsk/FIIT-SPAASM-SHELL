#pragma once

#include <arpa/inet.h>
#include "arguments.h"

#define KSHELL_SOCK_PATH "/tmp/kshell.sock"

struct connection {
	struct sockaddr* address;
	socklen_t addressLen;
	pthread_t* thread;
	int clientFd;
};

int server(struct arguments* args, char* target, int port, bool unix_socket);