#include "HostLink.h"
#include "Arduino.h"
#include <cstring>

HostLink hostLink;

void HostLink::begin(Stream &serial, SimpleCLI &cliRef) {
    _serial  = &serial;
    _cli     = &cliRef;
    _state   = State::IDLE;
    _lineLen = 0;
}

void HostLink::registerHandler(PacketCmd cmd, ProtocolHandler fn) {
    if (_handlerCount >= HOSTLINK_MAX_CMDS) return;
    _handlers[_handlerCount++] = { cmd, fn };
}

void HostLink::poll() {
    while (_serial->available()) {
        _feedByte(static_cast<uint8_t>(_serial->read()));
    }
}

void HostLink::_feedByte(uint8_t b) {
    switch (_state) {
    case State::IDLE:
        if (b == PROTOCOL_SYNC0) {
            _state = State::WAIT_SYNC1;
        } else {
            _feedCliByte(b);
        }
        return;

    case State::WAIT_SYNC1:
        if (b == PROTOCOL_SYNC1) {
            _state = State::LEN;
        } else {
            // The 0xAA we saw wasn't a real frame start -- it was CLI text.
            // Replay it there, then reprocess this byte from IDLE.
            _state = State::IDLE;
            _feedCliByte(PROTOCOL_SYNC0);
            _feedByte(b);
        }
        return;

    case State::LEN:
        if (b > PROTOCOL_MAX_PAYLOAD) {
            // Payload too large to fit our buffer -- abandon the frame.
            _state = State::IDLE;
            return;
        }
        _rxLen   = b;
        _rxIndex = 0;
        _state   = State::CMD;
        return;

    case State::CMD:
        _rxCmd = b;
        _state = (_rxLen > 0) ? State::PAYLOAD : State::CRC;
        return;

    case State::PAYLOAD:
        _rxPayload[_rxIndex++] = b;
        if (_rxIndex >= _rxLen) {
            _state = State::CRC;
        }
        return;

    case State::CRC:
        _onFrameComplete(b);
        _state = State::IDLE;
        return;
    }
}

void HostLink::_feedCliByte(uint8_t b) {
    if (!_cli) return;

    // Backspace/delete: erase the last buffered character, locally and on
    // the terminal (which doesn't do its own line editing over raw serial).
    if (b == '\b' || b == 0x7F) {
        if (_lineLen > 0) {
            _lineLen--;
            _serial->write("\b \b", 3);
        }
        return;
    }

    _serial->write(b); // echo, since a raw serial terminal won't do it for us

    if (b == '\r' || b == '\n') {
        if (b == '\r') _serial->write('\n');
        if (_lineLen > 0) {
            _cli->parse(_lineBuf, _lineLen);
            _lineLen = 0;
        }
        return;
    }

    if (_lineLen < HOSTLINK_MAX_LINE) {
        _lineBuf[_lineLen++] = static_cast<char>(b);
    }
}

void HostLink::_onFrameComplete(uint8_t crcByte) {
    uint8_t crcBuf[2 + PROTOCOL_MAX_PAYLOAD];
    crcBuf[0] = _rxLen;
    crcBuf[1] = _rxCmd;
    memcpy(crcBuf + 2, _rxPayload, _rxLen);
    if (protocolCrc8(crcBuf, (uint8_t)(2 + _rxLen)) != crcByte) {
        return; // corrupt frame, silently drop -- host is expected to retry/timeout
    }

    for (int i = 0; i < _handlerCount; i++) {
        if (static_cast<uint8_t>(_handlers[i].cmd) == _rxCmd) {
            uint8_t respPayload[PROTOCOL_MAX_PAYLOAD - 1];
            uint8_t respLen = 0;
            uint8_t status  = _handlers[i].fn(_rxPayload, _rxLen, respPayload, respLen);
            _sendResponse(_rxCmd, status, respPayload, respLen);
            return;
        }
    }

    _sendResponse(_rxCmd, static_cast<uint8_t>(PacketStatus::ERR_UNKNOWN_CMD), nullptr, 0);
}

void HostLink::_sendResponse(uint8_t cmd, uint8_t status, const uint8_t *payload, uint8_t len) {
    uint8_t crcBuf[3 + PROTOCOL_MAX_PAYLOAD];
    uint8_t totalLen = (uint8_t)(len + 1); // +1 for the status byte
    crcBuf[0] = totalLen;
    crcBuf[1] = cmd;
    crcBuf[2] = status;
    if (len > 0) memcpy(crcBuf + 3, payload, len);
    uint8_t crc = protocolCrc8(crcBuf, (uint8_t)(2 + totalLen));

    _serial->write(PROTOCOL_SYNC0);
    _serial->write(PROTOCOL_SYNC1);
    _serial->write(crcBuf, 2 + totalLen);
    _serial->write(crc);
}
