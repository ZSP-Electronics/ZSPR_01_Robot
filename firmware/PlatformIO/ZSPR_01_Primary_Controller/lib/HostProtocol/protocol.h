#pragma once
#include <cstdint>

// Wire format for the ESP32 <-> Raspberry Pi binary command protocol. Carried
// over the same native USB-CDC port as the human CLI (see HostLink) --
// distinguished from CLI text by the two-byte sync sequence below, which
// can't occur at the start of a typed command line.
//
// Frame layout, all requests (Pi -> ESP32) and responses (ESP32 -> Pi):
//   [0] SYNC0     0xAA
//   [1] SYNC1     0x55
//   [2] LEN       number of bytes in PAYLOAD (0-255)
//   [3] CMD       PacketCmd
//   [4..4+LEN-1]  PAYLOAD (request: command args. response: status byte
//                 followed by (LEN-1) bytes of result data)
//   [4+LEN]       CRC8 (Dallas/Maxim, poly 0x31, init 0x00) over
//                 LEN, CMD, and PAYLOAD

static constexpr uint8_t PROTOCOL_SYNC0 = 0xAA;
static constexpr uint8_t PROTOCOL_SYNC1 = 0x55;
static constexpr uint8_t PROTOCOL_MAX_PAYLOAD = 200;

enum class PacketCmd : uint8_t {
    PING              = 0x00,
    CAPS_QUERY        = 0x01, // resp: capability bitmask (u16 LE), see RobotCapability
    EVENT_POST        = 0x02, // payload: titleLen(u8), title bytes, command(u16 LE, a
                               // PacketCmd value), dataLen(u8), data bytes
                               // -> resp: assigned event id (u32 LE). Enqueues `command`
                               // to run on a later loop() tick via the same dispatch
                               // path as a synchronous frame -- this response only
                               // acknowledges the enqueue, not the queued command's
                               // own result (which is not pushed back to the host).

    SERVO_SET_POS     = 0x10, // payload: id, posLo, posHi, speedLo, speedHi, acc
    SERVO_SET_SPEED   = 0x11, // payload: id, speedLo, speedHi (s16), acc
    SERVO_SET_TORQUE  = 0x12, // payload: id, enable
    SERVO_READ        = 0x13, // payload: id -> resp: pos(s16), speed(s16), load(s16), voltage(u8), temp(s16) [9 bytes]

    TOF_READ           = 0x20, // resp: 64x distance_mm (u16 LE), 8x8 zone-major order

    IMU_READ           = 0x30, // resp: ax,ay,az,gx,gy,gz as float32 LE
    MAG_READ            = 0x31, // resp: mx,my,mz as float32 LE (uT)
    COMPASS_HEADING     = 0x32, // resp: heading_deg as float32 LE

    CURRENT_READ_SYS   = 0x40, // resp: bus_mV (u16), current_mA (s16), power_mW (u16)
    CURRENT_READ_MOTOR  = 0x41, // same layout as CURRENT_READ_SYS

    TOUCH_READ          = 0x50, // resp: key bitmask (u8), any_touched (u8)

    SD_LIST             = 0x60, // payload: path (nul-terminated) -> resp: newline-joined names
    SD_READ              = 0x61, // payload: path -> resp: file bytes
    SD_WRITE             = 0x62, // payload: path\0 + data -> resp: bytes written (u32)
    SD_DELETE            = 0x63, // payload: path -> resp: (none)
};

// One bit per ENABLE_* peripheral flag (see src/peripheral_config.h); queried
// via PacketCmd::CAPS_QUERY so a host can discover what this robot body
// supports before trying to use it.
enum class RobotCapability : uint16_t {
    SERVO         = 1 << 0,
    TOF           = 1 << 1,
    CURRENT_SENSE = 1 << 2,
    TOUCH         = 1 << 3,
    IMU_COMPASS   = 1 << 4,
    SD            = 1 << 5,
    DISPLAY_PANEL = 1 << 6, // named to avoid colliding with Arduino.h's `#define DISPLAY 0x1`
    BUZZER        = 1 << 7,
    BATTERY       = 1 << 8,
};

// Status byte, always the first byte of a response payload.
enum class PacketStatus : uint8_t {
    OK               = 0x00,
    ERR_UNKNOWN_CMD  = 0x01,
    ERR_BAD_ARGS     = 0x02,
    ERR_DISABLED     = 0x03, // peripheral compiled out via ENABLE_* flag
    ERR_HW_FAULT     = 0x04, // sensor/actuator did not respond
};

inline uint8_t protocolCrc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}
