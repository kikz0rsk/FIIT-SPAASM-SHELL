#pragma once

struct command {
	char* rawCommand;

	char** arguments;
	int numArguments;

	char* inputRedirect;
	char* outputRedirect;
	struct command* pipeRedirect;

	struct command* nextCommand;
};

void free_command(struct command* cmd);
void append_command(struct command** source, struct command* append);
char* remove_whitespaces(char* input);

char* strip_comments(char* original);
struct command* parse_input(char* ln);