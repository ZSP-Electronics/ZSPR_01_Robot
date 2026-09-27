#pragma once

#include <Arduino.h>
#include <SPI.h>

// ESP32 Arduino core provides its own FS.h/File; SdFat detects that and
// skips its own conflicting File/SdFat typedef aliases. We only ever use
// SdFat's own SdFs/FsFile types directly (see SDCard_Module::File below),
// so that's fine -- just silence the #warning about it.
#define DISABLE_FS_H_WARNING
#include <SdFat.h>
#include "Universal_Module.h"
#include "hardware_config.h"
#include "board_io.h"

// SdFat is built with SD_CHIP_SELECT_MODE=2 (see platformio.ini build_flags),
// which tells it not to define sdCsInit()/sdCsWrite() itself and instead
// expect the application to provide them. On this board the SD card's CS
// pin lives on the IO expander rather than a native ESP32 GPIO, so route
// both through the same board_pinMode/board_digitalWrite dispatcher every
// other peripheral in this project uses. `inline` gives these external
// linkage that safely collapses across translation units, same as the
// dispatcher functions in board_io.h.
//
// `__attribute__((used))` is required here: nothing in this translation
// unit calls these directly (only SdSpiCard.cpp, in the separately-compiled
// SdFat library, does), so without it GCC sees an unused `inline` function
// and drops it, leaving SdSpiCard.cpp's reference unresolved at link time.
inline __attribute__((used)) void sdCsInit(SdCsPin_t pin)
{
    board_pinMode(pin, OUTPUT);
}

inline __attribute__((used)) void sdCsWrite(SdCsPin_t pin, bool level)
{
    board_digitalWrite(pin, level ? HIGH : LOW);
}

// Thin wrapper around SdFat that mirrors the shape of the stock Arduino SD
// library (begin/exists/open/mkdir/remove/rmdir, File-like read/write/print),
// but drives its chip select through board_pinMode/board_digitalWrite so it
// works whether BOARD_SDMMC_CS is a native GPIO or an IO-expander pin.
class SDCard_Module : public Universal_Module
{
public:
    using File = FsFile;

    SDCard_Module(bool enable) : Universal_Module(enable) {}

    return_codes_t setup() override
    {
        if (_enabled)
        {
            _mounted = mount();
            if (!_mounted)
                return ERROR;
        }
        return SUCCESS;
    }

    return_codes_t update() override
    {
        if (_enabled)
        {
            _cardPresent = read_card_detect();
        }
        return SUCCESS;
    }

    /********************/
    /* SD CARD COMMANDS */
    /********************/

    // Raw state of BOARD_SDMMC_DET, as last sampled by update(). Board wiring
    // for this line's active level hasn't been confirmed against hardware
    // yet -- treat this as the raw pin level, not a validated "card present"
    // boolean.
    bool get_card_detect_raw(void) const { return _cardPresent; }

    bool is_mounted(void) const { return _mounted; }

    // (Re)mounts the volume. Safe to call again after a card is swapped.
    bool mount(void)
    {
        SdSpiConfig cfg(BOARD_SDMMC_CS, SHARED_SPI, SD_SCK_MHZ(16), &SPI);
        _mounted = _sd.begin(cfg);
        return _mounted;
    }

    bool exists(const char *path) { return _sd.exists(path); }
    bool mkdir(const char *path) { return _sd.mkdir(path); }
    bool remove(const char *path) { return _sd.remove(path); }
    bool rmdir(const char *path) { return _sd.rmdir(path); }
    bool rename(const char *oldPath, const char *newPath) { return _sd.rename(oldPath, newPath); }

    File open(const char *path, oflag_t oflag = O_RDONLY) { return _sd.open(path, oflag); }

    // Total/free space in KB, derived (filtered) from the raw cluster counts.
    uint64_t free_space_kb(void)
    {
        int32_t freeClusters = _sd.freeClusterCount();
        if (freeClusters < 0)
            return 0;
        return ((uint64_t)freeClusters * _sd.bytesPerCluster()) / 1024;
    }

    uint64_t total_space_kb(void)
    {
        return ((uint64_t)_sd.clusterCount() * _sd.bytesPerCluster()) / 1024;
    }

    uint8_t fat_type(void) { return _sd.fatType(); }

    // Prints each root-level entry (name, dir/file, size) to the given
    // stream. Returns the number of entries printed.
    uint16_t print_root_listing(Print &out)
    {
        File root = _sd.open("/");
        if (!root)
            return 0;

        uint16_t count = 0;
        File entry = root.openNextFile();
        while (entry)
        {
            char name[64];
            entry.getName(name, sizeof(name));
            if (entry.isDir())
            {
                out.printf("  [dir]  %s\r\n", name);
            }
            else
            {
                out.printf("  [file] %-32s %10llu bytes\r\n", name, (unsigned long long)entry.fileSize());
            }
            entry.close();
            count++;
            entry = root.openNextFile();
        }
        root.close();
        return count;
    }

    return_codes_t getData(uint16_t *data, int argc, char **argv) override
    {
        return SUCCESS;
    }

    return_codes_t setData(uint16_t *data, int argc, char **argv) override
    {
        return SUCCESS;
    }

private:
    SdFs _sd;
    bool _mounted = false;
    bool _cardPresent = false;

    bool read_card_detect(void)
    {
        return board_digitalRead(BOARD_SDMMC_DET);
    }
};
