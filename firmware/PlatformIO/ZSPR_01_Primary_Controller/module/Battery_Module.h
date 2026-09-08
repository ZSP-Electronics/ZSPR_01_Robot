#pragma once

#include <Arduino.h>
#include <cstring>
#include "Universal_Module.h"
#include "hardware_config.h"

// Set to 0 to disable the PING-based connectivity check entirely: setup()
// skips its blocking initial ping (and just reports SUCCESS), and update()
// stops sending the periodic 500ms re-ping. Useful when testing against a
// battery board that doesn't implement PING yet, or with none attached at
// all. The periodic 250ms current-power push is unaffected either way --
// with the check disabled, ping failures never accumulate, so it's never
// suspended. send_ping() itself stays available for a manual, on-demand
// check regardless of this setting.
#define DISABLE_BATTERY_PING_CHECK

// Wire format for the Primary Controller <-> Battery Controller board link,
// carried over the dedicated Serial2 UART (no CLI/HostLink text shares this
// port, so there's no need for HostProtocol's two-byte sync sequence -- a
// single start-of-frame byte is enough to resync after a dropped byte).
//
// Frame layout, both directions:
//   [0]           SOF   0x7E
//   [1]           CMD   BatteryCmd
//   [2]           LEN   number of bytes in DATA (0-BATTERY_MAX_PAYLOAD)
//   [3..3+LEN-1]  DATA
//   [3+LEN]       CRC8 (Dallas/Maxim, poly 0x31, init 0x00) over CMD, LEN, DATA
//
// BatteryPacket is the in-memory shape of one frame: a command ID, a pointer
// to its data bytes, the byte count, and the frame's CRC8. For TX, `data`
// points at caller-owned bytes; for RX, it points into this module's own
// internal buffer and is only valid for the duration of the onPacket() call.

static constexpr uint8_t BATTERY_SOF = 0x7E;
static constexpr uint8_t BATTERY_MAX_PAYLOAD = 32;

enum class BatteryCmd : uint8_t
{
    // Primary -> Battery: no data (request).
    // Battery -> Primary: data = "OK" (2 ASCII bytes)  (response).
    PING = 0x00,

    // Primary -> Battery. data: sysBus_mV(u16) sysCurrent_mA(s16) sysPower_mW(u16)
    //                           motorBus_mV(u16) motorCurrent_mA(s16) motorPower_mW(u16)  [12 bytes]
    SEND_CURRENT_POWER = 0x01,

    // Primary -> Battery: no data (request).
    // Battery -> Primary: data = battery voltage_mV (u16 LE)  [2 bytes] (response).
    REQUEST_VOLTAGE = 0x02,
};

struct BatteryPacket
{
    uint8_t cmdId;
    uint8_t *data;
    uint8_t dataLen;
    uint8_t crc;
};

class Battery_Module : public Universal_Module
{
public:
    Battery_Module(bool enable) : Universal_Module(enable) {}

    // Live readings needed for the periodic SEND_CURRENT_POWER push. Supplied
    // by a caller-provided callback (see attach_current_source()) rather than
    // this module including Current_Module.h directly, so it stays decoupled
    // from any specific current-sensor driver.
    struct CurrentReadings
    {
        uint16_t sysBus_mV;
        int16_t sysCurrent_mA;
        uint16_t sysPower_mW;
        uint16_t motorBus_mV;
        int16_t motorCurrent_mA;
        uint16_t motorPower_mW;
    };
    using CurrentSourceFn = CurrentReadings (*)();

    // Must be called (if at all) before setup()/update() run. If never set,
    // the periodic current-power push in update() is simply skipped.
    void attach_current_source(CurrentSourceFn fn) { _currentSource = fn; }

    return_codes_t setup() override
    {
        if (_enabled)
        {
            Serial2.begin(115200, SERIAL_8N1, BOARD_SERIAL_RX, BOARD_SERIAL_TX);
            // module's setup() runs.
#ifndef DISABLE_BATTERY_PING_CHECK
            send_ping();
            uint32_t start = millis();
            while (_pingPending && (millis() - start) < BATTERY_RESPONSE_TIMEOUT_MS)
                poll();

            if (_pingPending)
            {
                setPingResult(false);
                _pingPending = false;
            }
#else
            _linkStatus = SUCCESS; // ping check disabled -- assume the link is fine
#endif

            uint32_t now = millis();
            _lastPingMs = now;
            _lastCurrentPushMs = now;

            return _linkStatus;
        }
        return SUCCESS;
    }

    return_codes_t update() override
    {
        if (_enabled)
        {
            poll();

            uint32_t now = millis();

#ifndef DISABLE_BATTERY_PING_CHECK
            // Ping-based connectivity check, every BATTERY_PING_INTERVAL_MS.
            if (_pingPending && (now - _pingRequestMs >= BATTERY_RESPONSE_TIMEOUT_MS))
            {
                setPingResult(false); // no response within the timeout window
                _pingPending = false;
            }
            if (now - _lastPingMs >= BATTERY_PING_INTERVAL_MS)
            {
                _lastPingMs = now;
                send_ping();
            }
#endif

            // Periodic INA232 current/power push, every BATTERY_CURRENT_PUSH_INTERVAL_MS.
            // Suspended once the link has failed BATTERY_MAX_CONSECUTIVE_PING_FAILURES
            // pings in a row -- no point pushing data to a board that isn't answering.
            if (_currentSource && !current_push_suspended() &&
                (now - _lastCurrentPushMs >= BATTERY_CURRENT_PUSH_INTERVAL_MS))
            {
                _lastCurrentPushMs = now;
                CurrentReadings r = _currentSource();
                send_current_power(r.sysBus_mV, r.sysCurrent_mA, r.sysPower_mW,
                                    r.motorBus_mV, r.motorCurrent_mA, r.motorPower_mW);
            }
        }
        return SUCCESS;
    }

    /********************/
    /* BATTERY COMMANDS */
    /********************/

    // Sends a PING frame; a "OK" response updates link status to SUCCESS,
    // anything else (or no response within the timeout) sets it to ERROR.
    // Called automatically from setup() and every 500ms from update() --
    // exposed publicly in case a caller wants to force an out-of-cycle check.
    bool send_ping()
    {
        _pingPending = true;
        _pingRequestMs = millis();
        return sendPacket((uint8_t)BatteryCmd::PING, nullptr, 0);
        // Serial2.println("PING");
        // return 1;
    }

    // Result of the most recent ping (initial setup() ping, or the periodic
    // 500ms check in update()).
    return_codes_t get_link_status(void) const { return _linkStatus; }
    bool is_link_up(void) const { return _linkStatus == SUCCESS; }

    // True once BATTERY_MAX_CONSECUTIVE_PING_FAILURES pings have failed in a
    // row -- the periodic SEND_CURRENT_POWER push (update()) stops while this
    // is true, so as not to keep pushing data to a board that isn't there.
    // Clears the moment a single ping succeeds again.
    bool current_push_suspended(void) const { return _consecutivePingFailures >= BATTERY_MAX_CONSECUTIVE_PING_FAILURES; }

    // --- Debug / CLI introspection ---
    uint8_t get_consecutive_ping_failures(void) const { return _consecutivePingFailures; }
    bool ping_request_pending(void) const
    {
        return _pingPending && (millis() - _pingRequestMs < BATTERY_RESPONSE_TIMEOUT_MS);
    }

    // Sends the given INA232 system/motor rail readings to the battery
    // board. Caller supplies the raw values (e.g. from Current_Module) so
    // this module doesn't need to depend on any other peripheral driver.
    // Called automatically every 250ms from update() when a current source
    // is attached (see attach_current_source()); exposed publicly for an
    // on-demand push too.
    bool send_current_power(uint16_t sysBus_mV, int16_t sysCurrent_mA, uint16_t sysPower_mW,
                             uint16_t motorBus_mV, int16_t motorCurrent_mA, uint16_t motorPower_mW)
    {
        uint8_t payload[12];
        putU16(payload + 0, sysBus_mV);
        putS16(payload + 2, sysCurrent_mA);
        putU16(payload + 4, sysPower_mW);
        putU16(payload + 6, motorBus_mV);
        putS16(payload + 8, motorCurrent_mA);
        putU16(payload + 10, motorPower_mW);
        return sendPacket((uint8_t)BatteryCmd::SEND_CURRENT_POWER, payload, sizeof(payload));
    }

    // Sends a REQUEST_VOLTAGE frame. The response is picked up asynchronously
    // by poll() (called from update()) -- check voltage_request_pending() /
    // has_battery_voltage() / get_battery_voltage_mV() afterward.
    bool request_battery_voltage()
    {
        _voltagePending = true;
        _voltageRequestMs = millis();
        return sendPacket((uint8_t)BatteryCmd::REQUEST_VOLTAGE, nullptr, 0);
    }

    bool has_battery_voltage(void) const { return _haveVoltage; }
    uint16_t get_battery_voltage_mV(void) const { return _batteryVoltage_mV; }

    // True while a REQUEST_VOLTAGE is still awaiting a response, within the
    // timeout window. False once a response lands or the window expires.
    bool voltage_request_pending(void) const
    {
        return _voltagePending && (millis() - _voltageRequestMs < BATTERY_RESPONSE_TIMEOUT_MS);
    }

    return_codes_t getData(uint16_t *data, int argc, char **argv) override { return SUCCESS; }
    return_codes_t setData(uint16_t *data, int argc, char **argv) override { return SUCCESS; }

private:
    static constexpr uint32_t BATTERY_RESPONSE_TIMEOUT_MS = 200;
    static constexpr uint32_t BATTERY_PING_INTERVAL_MS = 500;
    static constexpr uint32_t BATTERY_CURRENT_PUSH_INTERVAL_MS = 250;
    static constexpr uint8_t BATTERY_MAX_CONSECUTIVE_PING_FAILURES = 2;

    CurrentSourceFn _currentSource = nullptr;

    return_codes_t _linkStatus = ERROR;
    uint8_t _consecutivePingFailures = 0;
    bool _pingPending = false;
    uint32_t _pingRequestMs = 0;
    uint32_t _lastPingMs = 0;
    uint32_t _lastCurrentPushMs = 0;

    // Updates link status from one ping's outcome and tracks the consecutive
    // failure streak that current_push_suspended() gates on.
    void setPingResult(bool ok)
    {
        _linkStatus = ok ? SUCCESS : ERROR;
        if (ok)
            _consecutivePingFailures = 0;
        else if (_consecutivePingFailures < 255)
            _consecutivePingFailures++;
    }

    enum class RxState
    {
        WAIT_SOF,
        WAIT_CMD,
        WAIT_LEN,
        WAIT_DATA,
        WAIT_CRC
    };

    RxState _rxState = RxState::WAIT_SOF;
    uint8_t _rxCmd = 0;
    uint8_t _rxLen = 0;
    uint8_t _rxIndex = 0;
    uint8_t _rxBuf[BATTERY_MAX_PAYLOAD];

    bool _haveVoltage = false;
    uint16_t _batteryVoltage_mV = 0;
    bool _voltagePending = false;
    uint32_t _voltageRequestMs = 0;

    static void putU16(uint8_t *b, uint16_t v)
    {
        b[0] = (uint8_t)(v & 0xFF);
        b[1] = (uint8_t)(v >> 8);
    }
    static void putS16(uint8_t *b, int16_t v) { putU16(b, (uint16_t)v); }
    static uint16_t getU16(const uint8_t *b) { return (uint16_t)(b[0] | (b[1] << 8)); }

    static uint8_t crc8(const uint8_t *data, uint8_t len)
    {
        uint8_t crc = 0x00;
        for (uint8_t i = 0; i < len; i++)
        {
            crc ^= data[i];
            for (uint8_t bit = 0; bit < 8; bit++)
                crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
        return crc;
    }

    bool sendPacket(uint8_t cmdId, const uint8_t *data, uint8_t dataLen)
    {
        if (dataLen > BATTERY_MAX_PAYLOAD)
            return false;

        BatteryPacket pkt;
        pkt.cmdId = cmdId;
        pkt.data = const_cast<uint8_t *>(data);
        pkt.dataLen = dataLen;

        uint8_t crcBuf[2 + BATTERY_MAX_PAYLOAD];
        crcBuf[0] = pkt.cmdId;
        crcBuf[1] = pkt.dataLen;
        if (pkt.dataLen)
            memcpy(crcBuf + 2, pkt.data, pkt.dataLen);
        pkt.crc = crc8(crcBuf, 2 + pkt.dataLen);

        Serial2.write(BATTERY_SOF);
        Serial2.write(pkt.cmdId);
        Serial2.write(pkt.dataLen);
        if (pkt.dataLen)
            Serial2.write(pkt.data, pkt.dataLen);
        Serial2.write(pkt.crc);
        return true;
    }

    void poll()
    {
        while (Serial2.available())
        {
            uint8_t b = (uint8_t)Serial2.read();
            switch (_rxState)
            {
            case RxState::WAIT_SOF:
                if (b == BATTERY_SOF)
                    _rxState = RxState::WAIT_CMD;
                break;

            case RxState::WAIT_CMD:
                _rxCmd = b;
                _rxState = RxState::WAIT_LEN;
                break;

            case RxState::WAIT_LEN:
                _rxLen = b;
                _rxIndex = 0;
                if (_rxLen > BATTERY_MAX_PAYLOAD)
                    _rxState = RxState::WAIT_SOF; // can't fit -- resync
                else if (_rxLen == 0)
                    _rxState = RxState::WAIT_CRC;
                else
                    _rxState = RxState::WAIT_DATA;
                break;

            case RxState::WAIT_DATA:
                _rxBuf[_rxIndex++] = b;
                if (_rxIndex >= _rxLen)
                    _rxState = RxState::WAIT_CRC;
                break;

            case RxState::WAIT_CRC:
            {
                uint8_t crcBuf[2 + BATTERY_MAX_PAYLOAD];
                crcBuf[0] = _rxCmd;
                crcBuf[1] = _rxLen;
                if (_rxLen)
                    memcpy(crcBuf + 2, _rxBuf, _rxLen);

                if (crc8(crcBuf, 2 + _rxLen) == b)
                {
                    BatteryPacket pkt{_rxCmd, _rxBuf, _rxLen, b};
                    onPacket(pkt);
                }
                _rxState = RxState::WAIT_SOF;
                break;
            }
            }
        }
    }

    void onPacket(const BatteryPacket &pkt)
    {
        switch ((BatteryCmd)pkt.cmdId)
        {
        case BatteryCmd::PING:
            setPingResult(pkt.dataLen == 2 && pkt.data[0] == 'O' && pkt.data[1] == 'K');
            _pingPending = false;
            break;

        case BatteryCmd::REQUEST_VOLTAGE:
            if (pkt.dataLen >= 2)
            {
                _batteryVoltage_mV = getU16(pkt.data);
                _haveVoltage = true;
            }
            _voltagePending = false;
            break;

        default:
            break;
        }
    }
};
