/*
 * STM_CLI.h
 *
 * Small, dependency-free command line interpreter for the on-board UART.
 * Owns both directions of the interface:
 *   - receive: poll() drains a line from Serial and dispatches it
 *   - send:    unknown commands and command output are written back to Serial
 *
 * Commands are registered up front (fixed-size table, no heap growth once
 * cli_setup() has run) with a name, a callback, and optionally a set of
 * boolean flag arguments. Each callback receives a CLIArgs with whatever
 * positional values/flags followed the command name on the input line.
 */

#ifndef LIBRARIES_CLI_STMCLI_STM_CLI_H_
#define LIBRARIES_CLI_STMCLI_STM_CLI_H_

#include <string>
#include <vector>
#include "CLIArgs.h"
#include "CLICommand.h"

#define CLI_MAX_COMMANDS 16

class STM_CLI
{
public:
	STM_CLI();

	/* Registers a command and returns a reference to it so callers can chain
	 * setDescription()/addFlagArg() (see cli_setup() in main.cpp). */
	CLICommand& addCommand(const std::string &name, CLICallback callback, const std::string &description = "");

	/* Parses a single line, dispatching to the matching command's callback.
	 * Returns false (and writes an error to Serial) if the line was empty
	 * or named an unregistered command. */
	bool parse(const std::string &line);

	/* Drives the receive side: if a full line is waiting on Serial, reads it
	 * and parses it. Call this once per main loop iteration. */
	void poll();

	/* Human-readable listing of every registered command, used for a help
	 * command. */
	std::string toString() const;

private:
	CLICommand _commands[CLI_MAX_COMMANDS];
	size_t _commandCount;

	CLICommand* findCommand(const std::string &name);
	static void tokenize(const std::string &line, std::vector<std::string> &tokens);
};

#endif /* LIBRARIES_CLI_STMCLI_STM_CLI_H_ */
