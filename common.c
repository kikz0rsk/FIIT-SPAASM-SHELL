#include "common.h"

void safe_free(void** ptr) {
	free(*ptr);
	*ptr = NULL;
}