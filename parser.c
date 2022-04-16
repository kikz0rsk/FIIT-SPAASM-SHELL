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
	free(stripped);

	return commands;
}

struct command* parse_commands(char* ln) {
	char* input = remove_whitespaces(ln);

	struct command* commands = NULL;

	char* start = input;
	int length = 0;
	int maxIndex = strlen(input);
	int index = 0;
	char currentChar;
	int skipSpaces = true;
	while (index <= maxIndex) {
		if (input[index] == 0) {
			if (length > 0) {
				char* cmdString = calloc(length + 1, sizeof(char));
				memcpy(cmdString, start, length);
				struct command* cmd = parse_command(cmdString);
				append_command(&commands, cmd);
				safe_free((void**)&cmdString);
			}

			break;
		}

		currentChar = input[index];

		if (currentChar == ' ') {
			if (skipSpaces) {
				index++;
				start = start + 1;
				continue;
			}
		}
		else {
			skipSpaces = false;
		}

		if (currentChar == ';' || currentChar == '|') {
			char* cmdString = calloc(length + 1, sizeof(char));
			memcpy(cmdString, start, length);
			struct command* cmd = parse_command(cmdString);
			append_command(&commands, cmd);
			safe_free((void**)&cmdString);

			if (currentChar == ';') {
				start = start + length + 1;
				index++;
				length = 0;
				skipSpaces = 1;
				continue;
			}
			else {
				start = start + length + 1;
				cmd->pipeRedirect = parse_commands(start);
				break;
			}
		}

		index++;
		length++;
	}

	free(input);
	return commands;
}

struct command* parse_command(char* commandString) {
	struct command* command = calloc(1, sizeof(struct command));

	commandString = remove_whitespaces(commandString);
	command->rawCommand = commandString;

	char** arguments = calloc(50, sizeof(char*));
	int argumentsIndex = 0;

	char* currentStart = commandString;
	int length = 0;
	int index = 0;
	int maxIndex = strlen(commandString);
	char currentChar;
	int currentCharIndex = 0;
	while (index <= maxIndex) {
		currentChar = commandString[index];

		if (currentChar == '>' || currentChar == '<') {
			char prev = currentChar;
			/*char prev = currentChar;
			commandString[index] = 0;

			char* argument = remove_whitespaces(currentStart);
			arguments[argumentsIndex++] = argument;
			commandString[index] = prev;*/

			index++;
			currentStart = &(commandString[index]);
			char* redirectTarget = remove_whitespaces(currentStart);
			if (prev == '>') {
				command->outputRedirect = redirectTarget;
			}
			else if (prev == '<') {
				command->inputRedirect = redirectTarget;
			}
			break;
		} else if (currentChar == ' ' || currentChar == 0) {
			char* argument = calloc(length + 1, sizeof(char));
			memcpy(argument, currentStart, length);
			arguments[argumentsIndex++] = argument;
			
			length = 0;
			index++;
			currentStart = &(commandString[index]);
			continue;
		}

		index++;
		length++;
	}

	command->arguments = arguments;
	command->numArguments = argumentsIndex;

	return command;
}

char* remove_whitespaces(char* input) {
	char* start = input;
	int length = strlen(input);
	while ((start[0] == ' ' || start[0] == '\t') && length > 0) {
		start += 1;
		length--;
	}

	char* end = start + length - 1;
	while ((end[0] == ' ' || end[0] == '\t') && length > 0) {
		end -= 1;
		length--;
	}

	char* output = calloc(length + 1, sizeof(char));
	memcpy(output, start, length);
	return output;
}

void free_command(struct command* cmd)
{
	for (int i = 0; i < cmd->numArguments; i++) {
		free(cmd->arguments[i]);
	}
	free(cmd->rawCommand);
	free(cmd->inputRedirect);
	free(cmd->outputRedirect);

	if (cmd->pipeRedirect != NULL) {
		free_command(cmd->pipeRedirect);
	}
}

void append_command(struct command** source, struct command* append)
{
	if (*source == NULL) {
		*source = append;
		return;
	}

	struct command* cmd = *source;
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
			if (input[index + 1] == '#') {	// if this is space before hashtag, end was at previous iteration
				break;
			}

			if (start != NULL) {	// If we have already reached start increment length, as it is a part of command
				length++;
			}

			index++;
			continue;
		}

		if (input[index] == '#') {
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