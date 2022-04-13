#include "common.h"
#include "arguments.h"
#include <string.h>

bool is_flag_set(struct arguments* args, char* flag) {
	for (int i = 1; i < args->count; i++) {
		if (strstr(args->args[i], flag) != NULL) {
			return true;
		}
	}

	return false;
}

char* get_flag_value(struct arguments* args, char* flag) {
	for (int i = 1; i < args->count; i++) {
		if (strcmp(args->args[i], flag) == 0 && i < args->count - 1) {
			return args->args[i + 1];
		}
	}

	return NULL;
}