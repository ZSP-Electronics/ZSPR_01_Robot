// STUB -- see GeneratedCodeAnimations.h. Same shape as a real export with zero animations.
#include "GeneratedCodeAnimations.h"
#include "src/CodeCommandStreamDataSource.h"

namespace GeneratedCodeAnimations
{
    const char SETUP_DATA_0[] PROGMEM = "\n";
    const char *const SETUP_DATAARRAY[] PROGMEM = {SETUP_DATA_0};

    CommandStream *GenerateSetupCommandStream()
    {
        return new CommandStream(new CodeCommandStreamDataSource(SETUP_DATAARRAY, 1));
    }

    CommandStream *GenerateCommandStreamByIndex(byte)
    {
        return nullptr;
    }

    byte getAnimationCount()
    {
        return 0;
    }

    const uint16_t *getConfigValues(uint8_t)
    {
        return nullptr;
    }
}
