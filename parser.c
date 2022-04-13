#include "parser.h"
#include "common.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct command* parse_commands(char* ln);
struct command* parse_command(char* commandString);

struct command* parse_input(char* ln) {
	char* stripped = strip_comments(ln);

	if (strlen(stripped) == 0) {
		return NULL;
	}

	struct command* commands = parse_commands(stripped);

	return commands;
}

struct command* parse_commands(char* ln) {
	struct command* commands = NULL;

	char* start = ln;
	int length = 0;
	int maxIndex = strlen(ln);
	int index = 0;
	char currentChar;
	while ((currentChar = ln[index]) != 0 && index <= maxIndex) {
		if (currentChar == ' ') {
			if (ln[index + 1] == ';') {
				char* cmdString = calloc(length + 1, sizeof(char));
				memcpy(cmdString, start, length);
				struct command* cmd = parse_command(cmdString);

				if (commands == NULL) {
					commands = cmd;
				}
				else {
					append_command(commands, cmd);
				}

				safe_free((void**)&cmdString);

				start = start + length + 3;
				index += 2;
				length = 0;
				continue;
			}
		}

		if (currentChar == ';') {
			char* cmdString = calloc(length + 1, sizeof(char));
			memcpy(cmdString, start, length);
			struct command* cmd = parse_command(cmdString);

			if (commands == NULL) {
				commands = cmd;
			}
			else {
				append_command(commands, cmd);
			}

			safe_free((void**)&cmdString);

			start = start + length + 1;
			index++;
			length = 0;
			continue;
		}

		index++;
		length++;
	}

	char* cmdString = calloc(length + 1, sizeof(char));
	memcpy(cmdString, start, length);
	struct command* cmd = parse_command(cmdString);

	if (commands == NULL) {
		commands = cmd;
	}
	else {
		append_command(commands, cmd);
	}

	safe_free((void**)&cmdString);

	return commands;
}

struct command* parse_command(char* commandString) {
	struct command* command = calloc(1, sizeof(struct command));

	char* raw = calloc(strlen(commandString) + 1, sizeof(char));
	strcpy(raw, commandString);

	char** arguments = calloc(50, sizeof(char*));
	int index = 0;
	// TODO redirect detection

	char* token = strtok(commandString, " ");
	if (token != NULL) {
		char* str = calloc(strlen(token) + 1, sizeof(char));
		strcpy(str, token);
		arguments[index++] = str;
	}

	token = strtok(NULL, " ");
	while (token != NULL) {
		char* str = calloc(strlen(token) + 1, sizeof(char));
		strcpy(str, token);
		arguments[index++] = str;
		token = strtok(NULL, " ");
	}

	command->rawCommand = raw;
	command->arguments = arguments;
	command->numArguments = index;

	return command;
}

void free_command(struct command* cmd)
{
	for (int i = 0; i < cmd->numArguments; i++) {
		free(cmd->arguments[i]);
	}
	free(cmd->rawCommand);
	free(cmd->inputRedirectTarget);
	free(cmd->outputRedirectTarget);
}

void append_command(struct command* source, struct command* append)
{
	struct command* cmd = source;
	while (cmd->nextCommand != NULL) { cmd = cmd->nextCommand; }
	cmd->nextCommand = append;
}

char* strip_comments(char* input) {
	int length = 0;
	char* start = NULL;

	int index = 0;
	char currentChar;
	while ((currentChar = input[index]) != 0) {	// Until we reach end of string
		if (currentChar == 9 || currentChar == ' ') {	// If current character is a space or tabulator
			if (input[index + 1] == 35) {	// if this is space before hashtag, end was at previous iteration
				break;
			}

			if (start != NULL) {	// If we have already reached start increment length, as it is a part of command
				length++;
			}

			index++;
			continue;
		}

		if (input[index] == 35) {
			break;
		}

		if (start == NULL) {
			start = input + index;
		}
		length++;
		index++;
	}

	char* command = calloc(length + 1, sizeof(char));
	if (start != NULL) {
		memcpy(command, start, length);
	}
	return command;
}