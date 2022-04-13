#pragma once

#include "arguments.h"
#include "common.h"

int client(struct arguments* args, char* target, int port, bool unix_socket);