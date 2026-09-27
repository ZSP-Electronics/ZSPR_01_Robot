#pragma once

#include <cstdint>
#include <cstring>
#include "Serial.h"

/*
 * Battery_Link.h
 *
 * Battery Controller side of the Primary Controller <-> Battery Controller
 * link. This is the mirror of the Primary's module/Battery_Module.h: same
 * wire format, opposite roles -- this board *receives* SEND_CURRENT_POWER and
 * *answers* REQUEST_VOLTAGE.
 *
 * The link runs over this board's single UART (the global `Serial`, USART1).
 * As on the Primary side, no CLI/text traffic shares this port, so a single
 * start-of-frame byte is enough to resync after a dropped byte -- there is no
 * need for a two-byte sync sequence.
 *
 * Frame layout, both directions:
 *   [0]           SOF   0x7E
 *   [1]           CMD   BatteryCmd
 *   [2]           LEN   number of bytes in DATA (0-BATTERY_MAX_PAYLOAD)
 *   [3..3+LEN-1]  DATA
 *   [3+LEN]       CRC8 (Dallas/Maxim, poly 0x31, init 0x00) over CMD, LEN, DATA
 */

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

class Battery_Link
{
public:
    Battery_Link() = default;

    // Latest INA232 system/motor rail readings pushed from the Primary via
    // SEND_CURRENT_POWER. Contents are meaningful once has_current_power()
    // returns true.
    struct RailReadings
    {
        uint16_t sysBus_mV = 0;
        int16_t sysCurrent_mA = 0;
        uint16_t sysPower_mW = 0;
        uint16_t motorBus_mV = 0;
        int16_t motorCurrent_mA = 0;
        uint16_t motorPower_mW = 0;
    };

    // Value answered to REQUEST_VOLTAGE. The main loop refreshes this from the
    // charger's VBAT ADC once per battery-status update; a request that lands
    // between updates is answered with the last cached reading.
    void set_battery_voltage_mV(uint16_t mV) { _batteryVoltage_mV = mV; }
    uint16_t battery_voltage_mV(void) { return _batteryVoltage_mV; }

    bool has_current_power(void) { return _haveRails; }
    const RailReadings &rails(void) const { return _rails; }

    // Drains whatever bytes are waiting on Serial, dispatching any complete
    // frames (storing rail data, answering voltage requests). Call once per
    // main loop iteration.
    void poll()
    {
        while (Serial.available())
        {
            feed((uint8_t)Serial.read());
        }
    }

private:
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

    uint16_t _batteryVoltage_mV = 0;
    RailReadings _rails;
    bool _haveRails = false;

    static void putU16(uint8_t *b, uint16_t v)
    {
        b[0] = (uint8_t)(v & 0xFF);
        b[1] = (uint8_t)(v >> 8);
    }
    static uint16_t getU16(const uint8_t *b) { return (uint16_t)(b[0] | (b[1] << 8)); }
    static int16_t getS16(const uint8_t *b) { return (int16_t)getU16(b); }

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

        uint8_t crcBuf[2 + BATTERY_MAX_PAYLOAD];
        crcBuf[0] = cmdId;
        crcBuf[1] = dataLen;
        if (dataLen)
            memcpy(crcBuf + 2, data, dataLen);

        uint8_t frame[3 + BATTERY_MAX_PAYLOAD + 1];
        frame[0] = BATTERY_SOF;
        frame[1] = cmdId;
        frame[2] = dataLen;
        if (dataLen)
            memcpy(frame + 3, data, dataLen);
        frame[3 + dataLen] = crc8(crcBuf, 2 + dataLen);

        Serial.write(frame, (size_t)(4 + dataLen));
        return true;
    }

    void feed(uint8_t b)
    {
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
                onPacket(_rxCmd, _rxBuf, _rxLen);

            _rxState = RxState::WAIT_SOF;
            break;
        }
        }
    }

    void onPacket(uint8_t cmdId, const uint8_t *data, uint8_t len)
    {
        switch ((BatteryCmd)cmdId)
        {
        case BatteryCmd::PING:
        {
            const uint8_t ok[2] = {'O', 'K'};
            sendPacket((uint8_t)BatteryCmd::PING, ok, sizeof(ok));
            break;
        }

        case BatteryCmd::SEND_CURRENT_POWER:
            if (len >= 12)
            {
                _rails.sysBus_mV = getU16(data + 0);
                _rails.sysCurrent_mA = getS16(data + 2);
                _rails.sysPower_mW = getU16(data + 4);
                _rails.motorBus_mV = getU16(data + 6);
                _rails.motorCurrent_mA = getS16(data + 8);
                _rails.motorPower_mW = getU16(data + 10);
                _haveRails = true;
            }
            break;

        case BatteryCmd::REQUEST_VOLTAGE:
        {
            uint8_t payload[2];
            putU16(payload, _batteryVoltage_mV);
            sendPacket((uint8_t)BatteryCmd::REQUEST_VOLTAGE, payload, sizeof(payload));
            break;
        }

        default:
            break;
        }
    }
};
