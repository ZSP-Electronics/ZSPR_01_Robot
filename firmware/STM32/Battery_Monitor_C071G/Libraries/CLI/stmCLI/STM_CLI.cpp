/*
 * STM_CLI.cpp
 */

#include "STM_CLI.h"
#include "Serial.h"

STM_CLI::STM_CLI() : _commandCount(0)
{
}

CLICommand& STM_CLI::addCommand(const std::string &name, CLICallback callback, const std::string &description)
{
	if (_commandCount >= CLI_MAX_COMMANDS)
	{
		/* Table is full -- this only happens if cli_setup() registers more
		 * commands than CLI_MAX_COMMANDS allows. Hand back the last slot
		 * unmodified rather than overwriting an already-registered command. */
		return _commands[CLI_MAX_COMMANDS - 1];
	}

	CLICommand &command = _commands[_commandCount++];
	command.configure(name, callback, description);
	return command;
}

CLICommand* STM_CLI::findCommand(const std::string &name)
{
	for (size_t i = 0; i < _commandCount; i++)
	{
		if (_commands[i].getName() == name) return &_commands[i];
	}
	return nullptr;
}

void STM_CLI::tokenize(const std::string &line, std::vector<std::string> &tokens)
{
	std::string current;

	for (size_t i = 0; i < line.size(); i++)
	{
		char c = line[i];
		bool isSeparator = (c == ' ' || c == '\t' || c == '\r' || c == '\n');

		if (isSeparator)
		{
			if (!current.empty())
			{
				tokens.push_back(current);
				current.clear();
			}
		}
		else
		{
			current += c;
		}
	}

	if (!current.empty()) tokens.push_back(current);
}

bool STM_CLI::parse(const std::string &line)
{
	std::vector<std::string> tokens;
	tokenize(line, tokens);

	if (tokens.empty()) return false;

	CLICommand *command = findCommand(tokens[0]);
	if (command == nullptr)
	{
		Serial.print("Unknown command: ");
		Serial.println(tokens[0]);
		return false;
	}

	CLIArgs args(tokens[0]);
	for (size_t i = 1; i < tokens.size(); i++)
	{
		if (command->hasFlagDefined(tokens[i]))
		{
			args.addFlag(tokens[i]);
		}
		else
		{
			args.addArg(tokens[i]);
		}
	}

	command->invoke(args);
	return true;
}

void STM_CLI::poll()
{
	if (!Serial.available()) return;

	String input = Serial.readStringUntil('\n');
#ifdef DEBUG
	Serial.println("> " + input);
#endif

	parse(input);
}

std::string STM_CLI::toString() const
{
	std::string out;

	for (size_t i = 0; i < _commandCount; i++)
	{
		out += _commands[i].toString();
		out += "\r\n";
	}

	return out;
}
