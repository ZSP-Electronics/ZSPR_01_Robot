#include "BottangoServoBridge.h"

BottangoServoBridge::Channel BottangoServoBridge::_ch[BOTTANGO_MAX_CHANNELS];
BottangoServoSink *BottangoServoBridge::_sink = nullptr;
uint16_t BottangoServoBridge::_intervalMs = 20;
uint16_t BottangoServoBridge::_firstMoveMs = 300;
uint32_t BottangoServoBridge::_lastFlushMs = 0;
uint32_t BottangoServoBridge::_movesSent = 0;

void BottangoServoBridge::setSink(BottangoServoSink *sink)
{
    _sink = sink;
}

bool BottangoServoBridge::setCalibration(uint8_t id, const BottangoServoCal &cal)
{
    if (id >= BOTTANGO_MAX_CHANNELS || cal.usMax == cal.usMin)
        return false;
    _ch[id].cal = cal;
    return true;
}

bool BottangoServoBridge::getCalibration(uint8_t id, BottangoServoCal &cal)
{
    if (id >= BOTTANGO_MAX_CHANNELS)
        return false;
    cal = _ch[id].cal;
    return true;
}

void BottangoServoBridge::onPulse(uint8_t, uint8_t channel, uint16_t us)
{
    if (channel >= BOTTANGO_MAX_CHANNELS)
        return;
    Channel &c = _ch[channel];
    if (c.targetUs == us && c.everSent)
        return;
    c.targetUs = us;
    c.dirty = true;
}

void BottangoServoBridge::reset()
{
    for (auto &c : _ch)
    {
        c.dirty = false;
        c.everSent = false;
    }
}

int16_t BottangoServoBridge::pulseToPos(uint8_t id, Channel &c, uint16_t us)
{
    int16_t posMin = c.cal.posMin, posMax = c.cal.posMax;
    if (posMin < 0 || posMax < 0)
    {
        int16_t lo = 0, hi = 0;
        if (!_sink || !_sink->servoRange(id, lo, hi))
            return -1;
        if (posMin < 0)
            posMin = lo;
        if (posMax < 0)
            posMax = hi;
    }

    float span = (float)c.cal.usMax - (float)c.cal.usMin;
    float frac = ((float)us - (float)c.cal.usMin) / span;
    frac = constrain(frac, 0.0f, 1.0f);
    if (c.cal.invert)
        frac = 1.0f - frac;
    return (int16_t)lroundf(posMin + frac * (float)(posMax - posMin));
}

void BottangoServoBridge::flush(uint32_t nowMs)
{
    if (!_sink || (nowMs - _lastFlushMs) < _intervalMs)
        return;
    _lastFlushMs = nowMs;

    BottangoServoMove moves[BOTTANGO_MAX_CHANNELS];
    uint8_t n = 0;

    for (uint8_t id = 0; id < BOTTANGO_MAX_CHANNELS; id++)
    {
        Channel &c = _ch[id];
        if (!c.dirty)
            continue;
        c.dirty = false;

        int16_t pos = pulseToPos(id, c, c.targetUs);
        if (pos < 0 || !_sink->hasServo(id))
            continue; // unknown servo: drop, nothing to move
        if (c.everSent && pos == c.lastPos)
            continue;

        uint16_t dt;
        uint16_t speed = 0;
        if (!c.everSent)
        {
            dt = _firstMoveMs; // unknown start point: move gently
        }
        else
        {
            uint32_t elapsed = nowMs - c.lastSendMs;
            dt = (uint16_t)constrain(elapsed, (uint32_t)_intervalMs, (uint32_t)250);
            uint32_t delta = abs((int)pos - (int)c.lastPos);
            speed = (uint16_t)min<uint32_t>(delta * 1000UL / dt, 65535UL);
            if (speed == 0)
                speed = 1;
        }

        moves[n++] = {id, pos, dt, speed};
        c.lastPos = pos;
        c.lastSendMs = nowMs;
        c.everSent = true;
    }

    if (n)
    {
        _sink->moveBatch(moves, n);
        _movesSent += n;
    }
}
