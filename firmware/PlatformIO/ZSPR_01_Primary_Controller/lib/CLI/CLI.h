#pragma once
#include <Arduino.h>
#include <Stream.h>
// Typed against Stream, not HardwareSerial: with ARDUINO_USB_CDC_ON_BOOT=1
// (native USB CDC, used on this board) the global `Serial` is an HWCDC
// instance, a sibling of HardwareSerial rather than a subclass -- Stream is
// their common base and still provides read/available/print/printf.

#define CLI_BUF_SIZE  256
#define CLI_MAX_CMDS   32
#define CLI_MAX_ARGS   16

typedef void (*cli_handler_t)(int argc, char **argv);

// Line-based command shell. Unlike a typical Arduino CLI, this class does not
// own Serial reads itself -- HostLink (see lib/HostProtocol) is the single
// reader of the shared USB-CDC port and forwards non-protocol bytes here one
// at a time via feed(), since the port also carries binary packets to/from
// the Raspberry Pi.
class CLI {
public:
    // Call after serial.begin(). Just prints the banner/prompt; does not
    // start a task or touch the port otherwise.
    void begin(Stream &serial = Serial);

    Stream &serial() const { return *_serial; }

    // Feed one input byte (from HostLink's read loop) into the line editor.
    void feed(char c);

    // Register a command. name is matched case-insensitively.
    void registerCmd(const char *name, const char *usage, const char *desc, cli_handler_t fn);

    void printHelp() const;

private:
    void _dispatch();

    char _buf[CLI_BUF_SIZE];
    int  _len = 0;

    struct CmdEntry {
        const char    *name;
        const char    *usage;
        const char    *desc;
        cli_handler_t  handler;
    };

    CmdEntry     _cmds[CLI_MAX_CMDS];
    int          _cmd_count = 0;
    Stream      *_serial = &Serial;
};

extern CLI cli;
