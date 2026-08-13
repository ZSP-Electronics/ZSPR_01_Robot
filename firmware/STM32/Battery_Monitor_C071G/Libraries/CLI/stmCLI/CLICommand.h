/*
 * CLICommand.h
 *
 * A single registered command: its name, help text, the flag arguments it
 * accepts, and the callback invoked when it is parsed off the input line.
 */

#ifndef LIBRARIES_CLI_STMCLI_CLICOMMAND_H_
#define LIBRARIES_CLI_STMCLI_CLICOMMAND_H_

#include <string>
#include <vector>
#include "CLIArgs.h"

typedef void (*CLICallback)(CLIArgs &args);

class CLICommand
{
public:
	CLICommand();

	void configure(const std::string &name, CLICallback callback, const std::string &description);

	CLICommand& setDescription(const std::string &description);
	CLICommand& addFlagArg(const std::string &flagName);

	const std::string& getName() const { return _name; }
	const std::string& getDescription() const { return _description; }
	bool isConfigured() const { return _callback != nullptr; }

	bool hasFlagDefined(const std::string &flagName) const;
	void invoke(CLIArgs &args) const;

	std::string toString() const;

private:
	std::string _name;
	std::string _description;
	CLICallback _callback;
	std::vector<std::string> _flagArgs;
};

#endif /* LIBRARIES_CLI_STMCLI_CLICOMMAND_H_ */
