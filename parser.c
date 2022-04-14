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
	int skipSpaces = true;
	while (index <= maxIndex) {
		if (ln[index] == 0) {
			if (length > 0) {
				char* cmdString = calloc(length + 1, sizeof(char));
				memcpy(cmdString, start, length);
				struct command* cmd = parse_command(cmdString);
				append_command(&commands, cmd);
				safe_free((void**)&cmdString);
			}

			break;
		}

		currentChar = ln[index];

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

		if (currentChar == ';') {
			char* cmdString = calloc(length + 1, sizeof(char));
			memcpy(cmdString, start, length);
			struct command* cmd = parse_command(cmdString);
			append_command(&commands, cmd);
			safe_free((void**)&cmdString);

			start = start + length + 1;
			index++;
			length = 0;
			skipSpaces = 1;
			continue;
		}

		index++;
		length++;
	}

	return commands;
}

struct command* parse_command(char* commandString) {
	struct command* command = calloc(1, sizeof(struct command));

	char* raw = calloc(strlen(commandString) + 1, sizeof(char));
	strcpy(raw, commandString);
	command->rawCommand = raw;

	char** arguments = calloc(50, sizeof(char*));
	int argumentsIndex = 0;

	char* currentStart = commandString;
	int length = 0;
	int index = 0;
	int maxIndex = strlen(commandString);
	char currentChar;
	int currentCharIndex = 0;
	while ((currentChar = commandString[currentCharIndex]) != 0 && currentCharIndex < maxIndex) {
		if (currentChar == '>' || (currentChar == ' ' && commandString[currentCharIndex + 1] == '>')) {
			char* argument = calloc(length + 1, sizeof(char));
			memcpy(argument, currentStart, length);
			arguments[argumentsIndex++] = argument;
			length = 0;

			if (currentChar == ' ' && commandString[currentCharIndex + 1] == '>') { currentCharIndex++; }
			currentCharIndex++;

			if (commandString[currentCharIndex] == ' ') { currentCharIndex++; }
			while (commandString[currentCharIndex] == ' ' && commandString[currentCharIndex] == 0) { currentCharIndex++; }
			currentStart = &(commandString[currentCharIndex]);

			while (commandString[currentCharIndex] != 0  && commandString[currentCharIndex] != ' ') {
				currentCharIndex++;
				length++;
			}

			char* redirect = calloc(length + 1, sizeof(char));
			memcpy(redirect, currentStart, length);
			command->outputRedirect = redirect;

			length = 0;
		}
		else if (currentChar == '<' || (currentChar == ' ' && commandString[currentCharIndex + 1] == '<')) {

		}
		currentChar = commandString[currentCharIndex];

		if (currentChar == ' ' || currentCharIndex + 1 == maxIndex) {
			if (commandString[currentCharIndex + 1] == 0 && commandString[currentCharIndex] != ' ') {
				length++;
			}
			char* argument = calloc(length + 1, sizeof(char));
			memcpy(argument, currentStart, length);
			arguments[argumentsIndex++] = argument;

			currentStart += length + 1;
			length = 0;
			currentCharIndex++;
			continue;
		}
		currentCharIndex++;
		length++;
	}

	command->arguments = arguments;
	command->numArguments = argumentsIndex;

	return command;
}

void free_command(struct command* cmd)
{
	for (int i = 0; i < cmd->numArguments; i++) {
		free(cmd->arguments[i]);
	}
	free(cmd->rawCommand);
	free(cmd->inputRedirect);
	free(cmd->outputRedirect);
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