#include "client.h"
#include "common.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>

int client(struct arguments* args, char* target, int port, bool unix_socket) {
	int* sockHandle = calloc(1, sizeof(int));
	struct sockaddr* addr;
	socklen_t sockAddrSize;

	if (unix_socket) {
		struct sockaddr_un* address = calloc(1, sizeof(struct sockaddr_un));
		*sockHandle = socket(AF_UNIX, SOCK_STREAM, 0);
		address->sun_family = AF_LOCAL;
		strcpy(address->sun_path, target);
		addr = (struct sockaddr*)address;
		sockAddrSize = sizeof(struct sockaddr_un);
	}
	else {
		struct sockaddr_in* address = calloc(1, sizeof(struct sockaddr_in));
		*sockHandle = socket(AF_INET, SOCK_STREAM, 0);
		address->sin_family = AF_INET;
		address->sin_port = htons(port);
		inet_pton(AF_INET, target, &(address->sin_addr.s_addr));
		addr = (struct sockaddr*)address;
		sockAddrSize = sizeof(struct sockaddr_in);
	}

	if (*sockHandle == -1) {
		printf("Failed to create socket: %s (%d)\n", strerror(errno), errno);
		return 1;
	}

	int res = connect(*sockHandle, addr, sockAddrSize);
	if (res < 0) {
		perror("Connect error");
		return 0;
	}

	fd_set rs;
	FD_ZERO(&rs);
	FD_SET(0, &rs);
	FD_SET(*sockHandle, &rs);

	char buffer[1024] = { 0 };
	int bytesRead = 0;
	while (select(*sockHandle + 1, &rs, NULL, NULL, NULL) > 0)
	{
		if (FD_ISSET(0, &rs))
		{
			bytesRead = read(0, &buffer, sizeof(buffer) - 1);
			if (buffer[bytesRead - 1] == '\n') buffer[bytesRead - 1] = 0;
			write(*sockHandle, buffer, bytesRead);
		}
		else if (FD_ISSET(*sockHandle, &rs))
		{
			bytesRead = read(*sockHandle, &buffer, sizeof(buffer) - 1);

			if (bytesRead == 0) {
				close(*sockHandle);
				break;
			}

			write(1, buffer, bytesRead);
		}

		FD_ZERO(&rs);
		FD_SET(0, &rs);
		FD_SET(*sockHandle, &rs);
	}

	safe_free((void*)&addr);
	safe_free((void*)&sockHandle);
}