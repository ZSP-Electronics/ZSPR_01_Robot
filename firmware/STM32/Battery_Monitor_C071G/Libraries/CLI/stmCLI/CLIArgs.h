/*
 * CLIArgs.h
 *
 * Holds the parsed data (positional values + flags) for a single command
 * invocation, and is handed to that command's callback.
 */

#ifndef LIBRARIES_CLI_STMCLI_CLIARGS_H_
#define LIBRARIES_CLI_STMCLI_CLIARGS_H_

#include <stdlib.h>
#include <string>
#include <vector>

class CLIArgs
{
public:
	explicit CLIArgs(const std::string &commandName) : _commandName(commandName) {}

	void addArg(const std::string &value) { _args.push_back(value); }
	void addFlag(const std::string &name) { _flags.push_back(name); }

	const std::string& getCommandName() const { return _commandName; }

	size_t getArgCount() const { return _args.size(); }

	std::string getArg(size_t index) const
	{
		if (index >= _args.size()) return std::string();
		return _args[index];
	}

	long getArgInt(size_t index, long defaultValue = 0) const
	{
		if (index >= _args.size()) return defaultValue;
		return strtol(_args[index].c_str(), nullptr, 10);
	}

	float getArgFloat(size_t index, float defaultValue = 0.0f) const
	{
		if (index >= _args.size()) return defaultValue;
		return strtof(_args[index].c_str(), nullptr);
	}

	bool hasFlag(const std::string &name) const
	{
		for (size_t i = 0; i < _flags.size(); i++)
		{
			if (_flags[i] == name) return true;
		}
		return false;
	}

private:
	std::string _commandName;
	std::vector<std::string> _args;
	std::vector<std::string> _flags;
};

#endif /* LIBRARIES_CLI_STMCLI_CLIARGS_H_ */
