/*
 * CLICommand.cpp
 */

#include "CLICommand.h"

CLICommand::CLICommand() : _callback(nullptr)
{
}

void CLICommand::configure(const std::string &name, CLICallback callback, const std::string &description)
{
	_name = name;
	_callback = callback;
	_description = description;
	_flagArgs.clear();
}

CLICommand& CLICommand::setDescription(const std::string &description)
{
	_description = description;
	return *this;
}

CLICommand& CLICommand::addFlagArg(const std::string &flagName)
{
	_flagArgs.push_back(flagName);
	return *this;
}

bool CLICommand::hasFlagDefined(const std::string &flagName) const
{
	for (size_t i = 0; i < _flagArgs.size(); i++)
	{
		if (_flagArgs[i] == flagName) return true;
	}
	return false;
}

void CLICommand::invoke(CLIArgs &args) const
{
	if (_callback != nullptr) _callback(args);
}

std::string CLICommand::toString() const
{
	std::string line = _name;

	for (size_t i = 0; i < _flagArgs.size(); i++)
	{
		line += " [";
		line += _flagArgs[i];
		line += "]";
	}

	line += " - ";
	line += _description;
	return line;
}
