#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>
#include <type_traits>

// Easy save/load of any data to the ESP32's non-volatile storage (NVS flash).
// Self-contained: it depends only on the Arduino core, nothing from this project.
//
//   #include "NVS_Store.h"
//
//   // Plain values and structs (anything trivially copyable):
//   NVS::save("brightness", (uint8_t)200);
//   uint8_t b = 255;                       // default if nothing was stored
//   NVS::load("brightness", b);            // true if a stored value was read
//   uint8_t b2 = NVS::get("brightness", (uint8_t)255);   // value-or-default form
//
//   // Arrays / raw bytes:
//   uint8_t ids[12];
//   NVS::saveArray("servo_ids", ids, 12);
//   NVS::loadArray("servo_ids", ids, 12);  // true only if exactly 12 elements were stored
//
//   // Strings:
//   NVS::saveString("name", "ZSPR-01");
//   String s; NVS::loadString("name", s);
//
//   NVS::exists("name");  NVS::erase("name");  NVS::eraseAll();
//
// Everything lives in one namespace (NVS_NAMESPACE) unless you pass another as the last
// argument. Keys are at most 15 characters. Every call opens and closes the namespace
// under a mutex, so they are safe from any task. Flash has limited write cycles: save when
// the user commits a change, not in a loop.
//
// Stored values carry no type information: load a key with the same type you saved it
// with. A size mismatch makes load() return false and leave the destination untouched.
//
// Define NVS_NAMESPACE (e.g. in build_flags: -DNVS_NAMESPACE=\"myrobot\") to change the
// default namespace for a different robot.
#ifndef NVS_NAMESPACE
#define NVS_NAMESPACE "zspr"
#endif

namespace NVS
{
    // --- raw bytes ---------------------------------------------------------

    bool saveBytes(const char *key, const void *data, size_t len, const char *ns = NVS_NAMESPACE);

    // True only if the stored blob is exactly `len` bytes; `data` is untouched otherwise.
    bool loadBytes(const char *key, void *data, size_t len, const char *ns = NVS_NAMESPACE);

    // --- strings -----------------------------------------------------------

    bool saveString(const char *key, const char *text, const char *ns = NVS_NAMESPACE);
    bool loadString(const char *key, String &out, const char *ns = NVS_NAMESPACE);

    // --- housekeeping ------------------------------------------------------

    bool exists(const char *key, const char *ns = NVS_NAMESPACE);
    bool erase(const char *key, const char *ns = NVS_NAMESPACE);

    // Removes every key in the namespace.
    bool eraseAll(const char *ns = NVS_NAMESPACE);

    // --- typed helpers (built on the byte functions) ------------------------

    template <typename T>
    inline bool save(const char *key, const T &value, const char *ns = NVS_NAMESPACE)
    {
        static_assert(std::is_trivially_copyable<T>::value, "NVS::save needs a trivially copyable type");
        return saveBytes(key, &value, sizeof(T), ns);
    }

    template <typename T>
    inline bool load(const char *key, T &value, const char *ns = NVS_NAMESPACE)
    {
        static_assert(std::is_trivially_copyable<T>::value, "NVS::load needs a trivially copyable type");
        return loadBytes(key, &value, sizeof(T), ns);
    }

    // The stored value, or `fallback` if nothing (of the right size) is stored.
    template <typename T>
    inline T get(const char *key, const T &fallback, const char *ns = NVS_NAMESPACE)
    {
        T v = fallback;
        load(key, v, ns);
        return v;
    }

    template <typename T>
    inline bool saveArray(const char *key, const T *items, size_t count, const char *ns = NVS_NAMESPACE)
    {
        static_assert(std::is_trivially_copyable<T>::value, "NVS::saveArray needs a trivially copyable type");
        return saveBytes(key, items, count * sizeof(T), ns);
    }

    template <typename T>
    inline bool loadArray(const char *key, T *items, size_t count, const char *ns = NVS_NAMESPACE)
    {
        static_assert(std::is_trivially_copyable<T>::value, "NVS::loadArray needs a trivially copyable type");
        return loadBytes(key, items, count * sizeof(T), ns);
    }
} // namespace NVS
