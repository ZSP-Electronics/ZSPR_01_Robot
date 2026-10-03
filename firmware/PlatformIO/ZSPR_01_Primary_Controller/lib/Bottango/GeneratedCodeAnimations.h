// STUB -- replace with the files produced by Bottango's
// "Save animations to code" export (GeneratedCodeAnimations.h / .cpp).
// With this stub the robot has no offline animations and live USB control
// from the Bottango app still works.

#ifndef GeneratedCodeAnimations_h
#define GeneratedCodeAnimations_h

#include <Arduino.h>
#include "src/CommandStream.h"

namespace GeneratedCodeAnimations
{
    byte getAnimationCount();
    const uint16_t *getConfigValues(byte animationIndex);
    CommandStream *GenerateSetupCommandStream();
    CommandStream *GenerateCommandStreamByIndex(byte animationIndex);
}

#endif // GeneratedCodeAnimations
