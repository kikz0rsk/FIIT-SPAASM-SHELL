#include "kshell.h"
#include "common.h"
#include "arguments.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "server.h"
#include "client.h"

int main(int argc, char** argv)
{
	struct arguments args = { argc, argv };

	if (args.count == 1 || is_flag_set(&args, "-h")) {
		char* help = get_help();
		puts(help);
		safe_free((void*)&help);
		return 0;
	}

	int port = 25566;
	char* customPort;
	if ((customPort = get_flag_value(&args, "-p")) != NULL) {
		port = atoi(customPort);
	}

	bool clientFlag = is_flag_set(&args, "-c");
	bool serverFlag = is_flag_set(&args, "-s");

	if (!clientFlag && !serverFlag) {	// Default mode is a server mode
		printf("No mode specified, running in server mode\n");
		serverFlag = true;
	}

	bool useLocalSocket = false;
	char* target = get_flag_value(&args, "-i");
	if (target == NULL) {
		if (is_flag_set(&args, "-p")) {
			target = "127.0.0.1";
		}
		else {
			target = get_flag_value(&args, "-u");
			useLocalSocket = true;
		}
	}
	if (target == NULL) {
		target = KSHELL_SOCK_PATH;
		useLocalSocket = true;
	}

	if (serverFlag) {
		server(&args, target, port, useLocalSocket);
		client(&args, target, port, useLocalSocket);
	}
	else {
		client(&args, target, port, useLocalSocket);
	}

	return 0;
}

char* get_help() {
	char helpString[] = { "kShell - interactive remote shell\n"
		"Author: Christian Danížek\n"
		"Usage: kshell <flags> <-c | -s>\n"
		"Flags:\n"
		"  -h  - display this help\n"
		"  -p <port>  - set port number to listen on (default 25566)\n"
		"  -u <path>  - set path to local socket to listen on (overrides -i and -p)\n"
		"  -i <ip>  - set IP address\n"
		"  -c  - client mode\n"
		"  -s  - server mode\n"
		"Commands:\n"
		"  help  - display this help\n"
		"  quit  - close remote session\n"
		"  halt  - close shell server\n" };
	char* help = malloc(sizeof(helpString) * sizeof(char));
	memcpy(help, helpString, sizeof(helpString));
	return help;
}