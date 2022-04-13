#pragma once

#include "arguments.h"

#define KSHELL_SOCK_PATH "/var/run/kshell.sock"

int server(struct arguments* args, char* target, int port, bool unix_socket);