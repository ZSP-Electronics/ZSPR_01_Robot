#include "CLI.h"
#include "Arduino.h"
#include <cstring>
#include <cctype>

CLI cli;

static constexpr const char *PROMPT = "\r\n> ";

void CLI::begin(Stream &serial) {
    _serial = &serial;
    _serial->print("\r\n=== Robi CLI ===  (type 'help' for commands)\r\n");
    _serial->print(PROMPT);
}

void CLI::registerCmd(const char *name, const char *usage, const char *desc, cli_handler_t fn) {
    if (_cmd_count >= CLI_MAX_CMDS) return;
    _cmds[_cmd_count++] = { name, usage, desc, fn };
}

void CLI::printHelp() const {
    _serial->println("\r\nAvailable commands:");
    _serial->println("  help                           Show this list");
    for (int i = 0; i < _cmd_count; i++) {
        _serial->printf("  %-32s %s\r\n",
                        _cmds[i].usage  ? _cmds[i].usage : _cmds[i].name,
                        _cmds[i].desc   ? _cmds[i].desc  : "");
    }
}

void CLI::feed(char c) {
    if (c == '\r' || c == '\n') {
        if (_len > 0) {
            _serial->println();
            _buf[_len] = '\0';
            _dispatch();
            _len = 0;
        }
        _serial->print(PROMPT);
        return;
    }

    // Backspace / DEL -- erase one character from terminal
    if (c == '\x7f' || c == '\x08') {
        if (_len > 0) {
            _len--;
            _serial->print("\x08 \x08");
        }
        return;
    }

    if (isprint(static_cast<unsigned char>(c)) && _len < CLI_BUF_SIZE - 1) {
        _buf[_len++] = c;
        _serial->write(c);  // local echo
    }
}

void CLI::_dispatch() {
    // Tokenise in-place by replacing spaces with NUL
    char *argv[CLI_MAX_ARGS];
    int   argc = 0;
    char *p    = _buf;

    while (*p && argc < CLI_MAX_ARGS) {
        while (*p == ' ') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = '\0';
    }
    if (argc == 0) return;

    if (strcasecmp(argv[0], "help") == 0) {
        printHelp();
        return;
    }

    for (int i = 0; i < _cmd_count; i++) {
        if (strcasecmp(argv[0], _cmds[i].name) == 0) {
            _cmds[i].handler(argc, argv);
            return;
        }
    }

    _serial->printf("Unknown command '%s'. Type 'help' for a list.\r\n", argv[0]);
}
