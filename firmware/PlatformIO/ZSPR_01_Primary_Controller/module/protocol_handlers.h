#pragma once

#include <cstring>
#include <math.h>
#include "HostLink.h"

inline void putU16(uint8_t *b, uint16_t v)
{
    b[0] = (uint8_t)(v & 0xFF);
    b[1] = (uint8_t)(v >> 8);
}

inline void putS16(uint8_t *b, int16_t v) { putU16(b, (uint16_t)v); }
inline void putF32(uint8_t *b, float v) { memcpy(b, &v, 4); }
inline uint16_t getU16(const uint8_t *b) { return (uint16_t)(b[0] | (b[1] << 8)); }
inline int16_t getS16(const uint8_t *b) { return (int16_t)getU16(b); }

inline uint8_t ok(uint8_t &respLen, uint8_t len)
{
    respLen = len;
    return (uint8_t)PacketStatus::OK;
}
inline uint8_t fail(uint8_t &respLen, PacketStatus s)
{
    respLen = 0;
    return (uint8_t)s;
}

inline uint8_t pingHandler(const uint8_t *payload, uint8_t len, uint8_t *respPayload, uint8_t &respLen)
{
    return ok(respLen, 0);
}