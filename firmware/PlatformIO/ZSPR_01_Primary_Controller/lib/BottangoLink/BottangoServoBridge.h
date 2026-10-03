#pragma once
#include <Arduino.h>

// One position command for the servo bus.
struct BottangoServoMove
{
    uint8_t id;
    int16_t pos;        // raw servo units (SC: 0-1023, ST: 0-4095)
    uint16_t timeMs;    // travel time to reach `pos` (drives SC servos)
    uint16_t speed;     // steps/s needed to reach `pos` in timeMs (drives ST servos); 0 = use default
};

// What the bridge needs from the servo hardware. Implemented by the robot
// firmware (module/Bottango_Module.h) so this library has no hardware deps.
class BottangoServoSink
{
public:
    virtual ~BottangoServoSink() {}
    // True if a servo with this ID answered on the bus.
    virtual bool hasServo(uint8_t id) = 0;
    // Usable raw position range of servo `id`. False if unknown.
    virtual bool servoRange(uint8_t id, int16_t &minPos, int16_t &maxPos) = 0;
    // Send all moves in as few bus transactions as possible.
    virtual void moveBatch(const BottangoServoMove *moves, uint8_t count) = 0;
};

// Per-servo mapping from Bottango's pulse width to servo position.
struct BottangoServoCal
{
    uint16_t usMin = 500;   // pulse (us) that maps to posMin
    uint16_t usMax = 2500;  // pulse (us) that maps to posMax
    int16_t posMin = -1;    // -1 = take from BottangoServoSink::servoRange()
    int16_t posMax = -1;
    bool invert = false;
};

#define BOTTANGO_MAX_CHANNELS 16 // a PCA9685 has 16 channels; channel number == servo ID

// Turns the virtual PCA9685's pulse stream into timed servo-bus moves.
// Bottango evaluates the animation curves on the controller and writes a new
// pulse width every loop. onPulse() just records the latest target; flush()
// (rate limited) converts every changed channel to a move whose duration is
// the time since that servo's previous move, so the servo glides between
// curve samples instead of stepping.
class BottangoServoBridge
{
public:
    static void setSink(BottangoServoSink *sink);

    // Min time between bus updates. Lower = smoother, more bus traffic.
    static void setUpdateIntervalMs(uint16_t ms) { _intervalMs = ms < 5 ? 5 : ms; }
    // Duration used for the first move of a servo after registration.
    static void setFirstMoveMs(uint16_t ms) { _firstMoveMs = ms; }

    static bool setCalibration(uint8_t id, const BottangoServoCal &cal);
    static bool getCalibration(uint8_t id, BottangoServoCal &cal);

    // Called by the virtual PCA9685 (Adafruit_PWMServoDriver::writeMicroseconds).
    static void onPulse(uint8_t i2cAddress, uint8_t channel, uint16_t us);

    // Call every loop after the Bottango driver has run.
    static void flush(uint32_t nowMs);

    // Forget move history (call when the driver deregisters effectors) so the
    // next registration gets a gentle first move.
    static void reset();

    static uint32_t movesSent() { return _movesSent; }

private:
    struct Channel
    {
        BottangoServoCal cal;
        uint16_t targetUs = 0;
        int16_t lastPos = 0;
        uint32_t lastSendMs = 0;
        bool dirty = false;
        bool everSent = false;
    };

    static int16_t pulseToPos(uint8_t id, Channel &c, uint16_t us);

    static Channel _ch[BOTTANGO_MAX_CHANNELS];
    static BottangoServoSink *_sink;
    static uint16_t _intervalMs;
    static uint16_t _firstMoveMs;
    static uint32_t _lastFlushMs;
    static uint32_t _movesSent;
};
