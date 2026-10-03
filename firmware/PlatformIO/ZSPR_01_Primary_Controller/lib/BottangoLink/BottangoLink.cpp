#include "BottangoLink.h"
#include "src/BottangoCore.h"
#include "src/PersistentConfigUtil.h"
#include <cstring>

BottangoLink::Port BottangoLink::_port;
Stream *BottangoLink::_real = nullptr;
bool BottangoLink::_driverStarted = false;
bool BottangoLink::_running = false;
int BottangoLink::_startIndex = -1;
bool BottangoLink::_startLoop = false;

// The driver's I/O (BOTTANGO_SERIAL) -- see lib/Bottango/ZSPR_PATCHES.md.
Stream &bottangoSerial()
{
    return BottangoLink::port();
}

// ---------------------------------------------------------------------------
// Port: a small ring buffer HostLink pushes complete lines into
// ---------------------------------------------------------------------------

bool BottangoLink::Port::push(const char *s, uint16_t n)
{
    if (n + 1 > SIZE - 1 - (uint16_t)available())
        return false; // would overflow; drop the whole line rather than splice it
    for (uint16_t i = 0; i < n; i++)
    {
        _buf[_head] = (uint8_t)s[i];
        _head = (_head + 1) & (SIZE - 1);
    }
    _buf[_head] = '\n';
    _head = (_head + 1) & (SIZE - 1);
    return true;
}

int BottangoLink::Port::read()
{
    if (_head == _tail)
        return -1;
    uint8_t b = _buf[_tail];
    _tail = (_tail + 1) & (SIZE - 1);
    return b;
}

int BottangoLink::Port::peek()
{
    return (_head == _tail) ? -1 : _buf[_tail];
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void BottangoLink::begin(Stream &port, BottangoServoSink &sink)
{
    _real = &port;
    _port.attach(port);
    BottangoServoBridge::setSink(&sink);
}

void BottangoLink::run()
{
    if (!_real || _running)
        return;
    _running = true;
    _port.clear();

    // Drop anything the CLI line that launched us left behind.
    while (_real->available())
        _real->read();

    if (!_driverStarted)
    {
        BottangoCore::bottangoSetup(); // offline mode: registers effectors, starts "play on start"
        _driverStarted = true;
    }
    else if (BottangoCore::isOffline() && BottangoCore::commandStreamProvider)
    {
        // Re-entry after quit: bring the exported setup back up.
        BottangoCore::initialized = true;
        BottangoCore::commandStreamProvider->runSetup();
    }

    if (_startIndex >= 0)
        playAnimation((uint8_t)_startIndex, _startLoop);

    char line[MAX_COMMAND_LENGTH];
    uint16_t len = 0;
    bool quit = false;

    while (!quit)
    {
        // Split the raw port into lines: "quit" ends the loop, everything
        // else goes to the driver (which expects whole lines ending in a newline).
        while (_real->available())
        {
            int c = _real->read();
            if (c == '\r' || c == '\n')
            {
                if (len == 4 && memcmp(line, "quit", 4) == 0)
                {
                    quit = true;
                    break;
                }
                if (len > 0)
                    _port.push(line, len);
                len = 0;
            }
            else if (len < sizeof(line) - 1)
            {
                line[len++] = (char)c;
            }
        }

        BottangoCore::bottangoLoop();
        BottangoServoBridge::flush(millis());
        delay(1); // yield to the RTOS / watchdog
    }

    BottangoCore::uninitialize(); // deregister effectors, reset parser state
    BottangoServoBridge::reset();
    _port.clear();
    _running = false;
}

void BottangoLink::_onControllerStarted()
{
    BottangoServoBridge::reset();
}

void BottangoLink::_onControllerStopped()
{
    BottangoServoBridge::reset();
}

// ---------------------------------------------------------------------------
// Offline animations
// ---------------------------------------------------------------------------

bool BottangoLink::isOffline()
{
    return BottangoCore::isOffline();
}

void BottangoLink::setOffline(bool offline)
{
    PersistentConfigUtil::setUseExportedCommandStream(offline);
    delay(50);
    ESP.restart();
}

uint8_t BottangoLink::animationCount()
{
    return GeneratedCodeAnimations::getAnimationCount();
}

bool BottangoLink::playAnimation(uint8_t index, bool loop)
{
    if (!_running || !isOffline() || BottangoCore::commandStreamProvider == nullptr ||
        index >= animationCount())
        return false;
    BottangoCore::commandStreamProvider->startCommandStream(index, loop);
    return true;
}
