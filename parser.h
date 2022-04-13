#pragma once

struct command {
	char* rawCommand;

	char** arguments;
	int numArguments;

	char* inputRedirectTarget;
	char* outputRedirectTarget;

	struct command* nextCommand;
};

void free_command(struct command* cmd);
void append_command(struct command* source, struct command* append);

char* strip_comments(char* original);
struct command* parse_input(char* ln);