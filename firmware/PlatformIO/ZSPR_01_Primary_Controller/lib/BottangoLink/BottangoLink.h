#pragma once
#include <Arduino.h>
#include <Stream.h>
#include "BottangoServoBridge.h"

// Glue between the Bottango desktop app / exported offline animations and the
// robot's SC servo bus.
//
//   Bottango app --USB--> BottangoLink::run() --> Bottango driver
//   exported animations (flash) ------------------------^   |
//                         virtual PCA9685 -> BottangoServoBridge -> BottangoServoSink -> servos
//
// Bottango is NOT part of the normal firmware loop. run() is a blocking loop,
// started on demand (CLI: `bottango run`), that owns the USB port until the
// host sends the line "quit". While it runs nothing else is serviced -- not
// HostLink, not the CLI, not the other peripherals.
class BottangoLink
{
public:
    // Call once after the servo bus is up. Only wires things together; the
    // Bottango driver itself is not started until the first run().
    static void begin(Stream &port, BottangoServoSink &sink);

    // Blocking. Feeds every line received on the port to the Bottango driver
    // and runs the driver (live control and, in offline mode, animation
    // playback) until a line "quit" arrives. Returns afterwards with the
    // driver's effectors deregistered; servos hold their last position.
    static void run();

    // --- offline animations ---
    static bool isOffline();                 // booted in exported-animation mode
    static void setOffline(bool offline);    // persists, then reboots
    static uint8_t animationCount();
    // Valid only while run() is active (i.e. from callbacks inside it).
    static bool playAnimation(uint8_t index, bool loop);
    // Index to start as soon as run() begins (offline mode); -1 = none.
    static void setStartAnimation(int index, bool loop) { _startIndex = index; _startLoop = loop; }

    static bool running() { return _running; }

    // --- internal, called from the Callbacks:: implementation ---
    static void _onControllerStarted();
    static void _onControllerStopped();

    // Stream the driver reads/writes (see bottangoSerial()).
    class Port : public Stream
    {
    public:
        void attach(Stream &real) { _real = &real; }
        bool push(const char *s, uint16_t n);
        void clear() { _head = _tail = 0; }

        int available() override { return (int)((_head - _tail) & (SIZE - 1)); }
        int read() override;
        int peek() override;
        size_t write(uint8_t b) override { return _real ? _real->write(b) : 0; }
        size_t write(const uint8_t *buf, size_t n) override { return _real ? _real->write(buf, n) : 0; }
        void flush() override { if (_real) _real->flush(); }

    private:
        static constexpr uint16_t SIZE = 512; // power of two, > 2x MAX_COMMAND_LENGTH
        uint8_t _buf[SIZE];
        volatile uint16_t _head = 0, _tail = 0;
        Stream *_real = nullptr;
    };

private:
    static Port _port;
    static Stream *_real;
    static bool _driverStarted;
    static bool _running;
    static int _startIndex;
    static bool _startLoop;

    friend Stream &bottangoSerial();
    static Port &port() { return _port; }
};
