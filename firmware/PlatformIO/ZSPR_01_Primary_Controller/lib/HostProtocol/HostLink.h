#pragma once
#include <Arduino.h>
#include <Stream.h>
#include "protocol.h"
#include "SimpleCLI.h"

#define HOSTLINK_MAX_CMDS 32
#define HOSTLINK_MAX_LINE 128

// Handler for one PacketCmd: reads `payload`/`len` (the request payload),
// fills `respPayload`/`respLen` with the result data, and returns the
// PacketStatus to report. respPayload has room for PROTOCOL_MAX_PAYLOAD - 1
// bytes (one byte of the response's LEN budget is the status byte itself).
typedef uint8_t (*ProtocolHandler)(const uint8_t *payload, uint8_t len,
                                    uint8_t *respPayload, uint8_t &respLen);

// Owns the single shared USB-CDC Serial port. Splits incoming bytes between
// binary PacketCmd frames (see protocol.h, used by the Raspberry Pi) and
// human-typed lines by watching for the two-byte sync sequence -- anything
// not part of a recognised frame is buffered a line at a time and handed to
// SimpleCLI::parse(), so the same command set answers identically whether
// it arrives as a binary frame or a typed CLI line over the same port.
class HostLink {
public:
    void begin(Stream &serial, SimpleCLI &cliRef);

    // Call every loop() iteration. Drains all available Serial bytes.
    void poll();

    void registerHandler(PacketCmd cmd, ProtocolHandler fn);

private:
    enum class State { IDLE, WAIT_SYNC1, LEN, CMD, PAYLOAD, CRC };

    void _feedByte(uint8_t b);
    void _feedCliByte(uint8_t b);
    void _onFrameComplete(uint8_t crcByte);
    void _sendResponse(uint8_t cmd, uint8_t status, const uint8_t *payload, uint8_t len);

    Stream    *_serial = &Serial;
    SimpleCLI *_cli    = nullptr;

    State   _state = State::IDLE;
    uint8_t _rxLen = 0;
    uint8_t _rxCmd = 0;
    uint8_t _rxIndex = 0;
    uint8_t _rxPayload[PROTOCOL_MAX_PAYLOAD];

    // Line buffer for the human CLI side, fed to SimpleCLI once per line.
    char    _lineBuf[HOSTLINK_MAX_LINE];
    uint8_t _lineLen = 0;

    struct HandlerEntry {
        PacketCmd       cmd;
        ProtocolHandler fn;
    };
    HandlerEntry _handlers[HOSTLINK_MAX_CMDS];
    int          _handlerCount = 0;
};

extern HostLink hostLink;
