#pragma once
#include "common.h"

struct arguments {
	int count;
	char** args;
};

bool is_flag_set(struct arguments* args, char* flag);
char* get_flag_value(struct arguments* args, char* flag);