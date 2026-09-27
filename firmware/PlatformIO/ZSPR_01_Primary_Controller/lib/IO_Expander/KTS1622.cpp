#include "KTS1622.h"

namespace
{
    // KTS1622 I2C register map (datasheet Table 2). Port 0 registers are listed;
    // port 1 is always base + 1 unless noted otherwise.
    constexpr uint8_t REG_INPUT_PORT0 = 0x00;
    constexpr uint8_t REG_OUTPUT_PORT0 = 0x02;
    constexpr uint8_t REG_POLARITY_PORT0 = 0x04;
    constexpr uint8_t REG_CONFIG_PORT0 = 0x06;
    constexpr uint8_t REG_DRIVE_STRENGTH_PORT0_LOW = 0x40; // pins 0-3, then +1 = pins 4-7, +2/+3 = port 1
    constexpr uint8_t REG_INPUT_LATCH_PORT0 = 0x44;
    constexpr uint8_t REG_PULL_ENABLE_PORT0 = 0x46;
    constexpr uint8_t REG_PULL_SELECT_PORT0 = 0x48;
    constexpr uint8_t REG_INT_MASK_PORT0 = 0x4A;
    constexpr uint8_t REG_INT_STATUS_PORT0 = 0x4C;
    constexpr uint8_t REG_OUTPUT_PORT_CONFIG = 0x4F; // bit0 = ODEN0, bit1 = ODEN1 (whole chip, not per-port pair)
    constexpr uint8_t REG_INT_EDGE_PORT0_LOW = 0x50; // 2 bits/pin, same grouping as drive strength
    constexpr uint8_t REG_INT_CLEAR_PORT0 = 0x54;
    constexpr uint8_t REG_INPUT_STATUS_PORT0 = 0x56;
    constexpr uint8_t REG_INDIV_PIN_OUTPUT_CONFIG_PORT0 = 0x58;
    constexpr uint8_t REG_SWITCH_DEBOUNCE_ENABLE_PORT0 = 0x5A;
    constexpr uint8_t REG_SWITCH_DEBOUNCE_COUNT = 0x5C;

    constexpr uint8_t I2C_GENERAL_CALL_ADDRESS = 0x00;
    constexpr uint8_t SOFTWARE_RESET_DATA = 0x06;
}

KTS1622::KTS1622(uint8_t i2cAddress, TwoWire &wirePort)
    : _wire(&wirePort), _address(i2cAddress) {}

void KTS1622::splitPin(uint8_t pin, uint8_t &port, uint8_t &bit)
{
    port = pin >> 3;
    bit = pin & 0x07;
}

bool KTS1622::begin()
{
    _wire->beginTransmission(_address);
    return _wire->endTransmission() == 0;
}

uint8_t KTS1622::readRegister(uint8_t reg)
{
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->endTransmission(false);
    _wire->requestFrom(_address, (uint8_t)1);
    return _wire->available() ? _wire->read() : 0;
}

void KTS1622::writeRegister(uint8_t reg, uint8_t value)
{
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(value);
    _wire->endTransmission();
}

void KTS1622::writeRegisterBit(uint8_t reg, uint8_t bit, bool value)
{
    uint8_t current = readRegister(reg);
    if (value)
    {
        current |= (1 << bit);
    }
    else
    {
        current &= ~(1 << bit);
    }
    writeRegister(reg, current);
}

void KTS1622::pinMode(uint8_t pin, uint8_t mode)
{
    // Matched by exact value rather than by bit-testing: the ESP32 Arduino core's
    // OUTPUT (0x03) shares its low bit with INPUT (0x01), so bitwise tests can't
    // tell them apart.
    uint8_t port, bit;
    splitPin(pin, port, bit);

    switch (mode)
    {
    case OUTPUT:
        writeRegisterBit(REG_CONFIG_PORT0 + port, bit, false);
        writeRegisterBit(REG_INDIV_PIN_OUTPUT_CONFIG_PORT0 + port, bit, false);
        break;
    case OUTPUT_OPEN_DRAIN:
        writeRegisterBit(REG_CONFIG_PORT0 + port, bit, false);
        writeRegisterBit(REG_INDIV_PIN_OUTPUT_CONFIG_PORT0 + port, bit, true);
        break;
    case INPUT_PULLUP:
        writeRegisterBit(REG_CONFIG_PORT0 + port, bit, true);
        writeRegisterBit(REG_PULL_ENABLE_PORT0 + port, bit, true);
        writeRegisterBit(REG_PULL_SELECT_PORT0 + port, bit, true);
        break;
    case INPUT_PULLDOWN:
        writeRegisterBit(REG_CONFIG_PORT0 + port, bit, true);
        writeRegisterBit(REG_PULL_ENABLE_PORT0 + port, bit, true);
        writeRegisterBit(REG_PULL_SELECT_PORT0 + port, bit, false);
        break;
    case INPUT:
    default:
        writeRegisterBit(REG_CONFIG_PORT0 + port, bit, true);
        writeRegisterBit(REG_PULL_ENABLE_PORT0 + port, bit, false);
        break;
    }
}

void KTS1622::digitalWrite(uint8_t pin, uint8_t value)
{
    uint8_t port, bit;
    splitPin(pin, port, bit);
    writeRegisterBit(REG_OUTPUT_PORT0 + port, bit, value != 0);
}

int KTS1622::digitalRead(uint8_t pin)
{
    uint8_t port, bit;
    splitPin(pin, port, bit);
    return (readRegister(REG_INPUT_PORT0 + port) >> bit) & 0x01;
}

void KTS1622::portMode(uint8_t port, uint8_t inputMask)
{
    writeRegister(REG_CONFIG_PORT0 + port, inputMask);
}

void KTS1622::writePort(uint8_t port, uint8_t value)
{
    writeRegister(REG_OUTPUT_PORT0 + port, value);
}

uint8_t KTS1622::readPort(uint8_t port)
{
    return readRegister(REG_INPUT_PORT0 + port);
}

void KTS1622::writeAllPorts(uint16_t value)
{
    writePort(0, value & 0xFF);
    writePort(1, (value >> 8) & 0xFF);
}

uint16_t KTS1622::readAllPorts()
{
    return readPort(0) | (readPort(1) << 8);
}

uint8_t KTS1622::readPortStatus(uint8_t port)
{
    return readRegister(REG_INPUT_STATUS_PORT0 + port);
}

uint16_t KTS1622::readAllPortsStatus()
{
    return readPortStatus(0) | (readPortStatus(1) << 8);
}

void KTS1622::setPolarityInversion(uint8_t pin, bool inverted)
{
    uint8_t port, bit;
    splitPin(pin, port, bit);
    writeRegisterBit(REG_POLARITY_PORT0 + port, bit, inverted);
}

void KTS1622::setInputLatch(uint8_t pin, bool latched)
{
    uint8_t port, bit;
    splitPin(pin, port, bit);
    writeRegisterBit(REG_INPUT_LATCH_PORT0 + port, bit, latched);
}

void KTS1622::setPullUpDown(uint8_t pin, bool pullUp)
{
    uint8_t port, bit;
    splitPin(pin, port, bit);
    writeRegisterBit(REG_PULL_ENABLE_PORT0 + port, bit, true);
    writeRegisterBit(REG_PULL_SELECT_PORT0 + port, bit, pullUp);
}

void KTS1622::disablePull(uint8_t pin)
{
    uint8_t port, bit;
    splitPin(pin, port, bit);
    writeRegisterBit(REG_PULL_ENABLE_PORT0 + port, bit, false);
}

void KTS1622::setOutputDriveStrength(uint8_t pin, KTS1622_DriveStrength strength)
{
    uint8_t port, bit;
    splitPin(pin, port, bit);
    // Each register packs 4 pins, 2 bits each: reg = base + port*2 + bit/4, shift = (bit%4)*2.
    uint8_t reg = REG_DRIVE_STRENGTH_PORT0_LOW + port * 2 + bit / 4;
    uint8_t shift = (bit % 4) * 2;
    uint8_t current = readRegister(reg);
    current = (current & ~(0b11 << shift)) | (strength << shift);
    writeRegister(reg, current);
}

void KTS1622::setPortOpenDrain(uint8_t port, bool openDrain)
{
    writeRegisterBit(REG_OUTPUT_PORT_CONFIG, port, openDrain);
}

void KTS1622::setPinOpenDrain(uint8_t pin, bool openDrain)
{
    uint8_t port, bit;
    splitPin(pin, port, bit);
    writeRegisterBit(REG_INDIV_PIN_OUTPUT_CONFIG_PORT0 + port, bit, openDrain);
}

void KTS1622::setInterruptEnabled(uint8_t pin, bool enabled)
{
    uint8_t port, bit;
    splitPin(pin, port, bit);
    writeRegisterBit(REG_INT_MASK_PORT0 + port, bit, !enabled);
}

void KTS1622::setInterruptEdge(uint8_t pin, KTS1622_InterruptEdge edge)
{
    uint8_t port, bit;
    splitPin(pin, port, bit);
    uint8_t reg = REG_INT_EDGE_PORT0_LOW + port * 2 + bit / 4;
    uint8_t shift = (bit % 4) * 2;
    uint8_t current = readRegister(reg);
    current = (current & ~(0b11 << shift)) | (edge << shift);
    writeRegister(reg, current);
}

uint16_t KTS1622::getInterruptStatus()
{
    return readRegister(REG_INT_STATUS_PORT0) | (readRegister(REG_INT_STATUS_PORT0 + 1) << 8);
}

void KTS1622::clearInterrupt(uint8_t pin)
{
    uint8_t port, bit;
    splitPin(pin, port, bit);
    writeRegister(REG_INT_CLEAR_PORT0 + port, 1 << bit);
}

void KTS1622::clearAllInterrupts()
{
    writeRegister(REG_INT_CLEAR_PORT0, 0xFF);
    writeRegister(REG_INT_CLEAR_PORT0 + 1, 0xFF);
}

void KTS1622::setSwitchDebounceEnabled(uint8_t pin, bool enabled)
{
    uint8_t port, bit;
    splitPin(pin, port, bit);
    writeRegisterBit(REG_SWITCH_DEBOUNCE_ENABLE_PORT0 + port, bit, enabled);
}

void KTS1622::setDebounceCount(uint8_t count)
{
    writeRegister(REG_SWITCH_DEBOUNCE_COUNT, count);
}

void KTS1622::resetAllDevicesOnBus(TwoWire &wirePort)
{
    wirePort.beginTransmission(I2C_GENERAL_CALL_ADDRESS);
    wirePort.write(SOFTWARE_RESET_DATA);
    wirePort.endTransmission();
}

// ---------------------------------------------------------------------------
// KTS1622Chain implementation (embedded here to keep helper with driver)
// ---------------------------------------------------------------------------

KTS1622_IO_Expander::KTS1622_IO_Expander(uint8_t numModules, const uint8_t addresses[], TwoWire &wirePort)
    : _numModules(std::min<uint8_t>(numModules, KTS1622_IO_Expander::MAX_MODULES)), _wire(&wirePort)
{
    for (uint8_t i = 0; i < KTS1622_IO_Expander::MAX_MODULES; ++i)
    {
        _modules[i] = nullptr;
    }
    for (uint8_t i = 0; i < _numModules; ++i)
    {
        _modules[i] = new KTS1622(addresses[i], wirePort);
    }
    _failedMask = 0;
}

KTS1622_IO_Expander::~KTS1622_IO_Expander()
{
    for (uint8_t i = 0; i < _numModules; ++i)
    {
        delete _modules[i];
        _modules[i] = nullptr;
    }
}

bool KTS1622_IO_Expander::begin(const uint8_t rst_pins[])
{
    bool ok = true;
    _failedMask = 0;
    for (uint8_t i = 0; i < _numModules; ++i)
    {
        if (_modules[i])
        {
            if (!_modules[i]->begin())
            {
                _failedMask |= 1U << i;
                ok = false;
            }
        }
        else
        {
            _failedMask |= 1U << i;
            ok = false;
        }
    }
    return ok;
}

bool KTS1622_IO_Expander::mapPin(uint8_t pin, uint8_t &moduleIndex, uint8_t &localPin) const
{
    uint16_t totalPins = uint16_t(_numModules) * 16u;
    if (pin >= totalPins)
        return false;

    moduleIndex = pin / 16u;
    localPin = pin % 16u;
    return true;
}

void KTS1622_IO_Expander::pinMode(uint8_t pin, uint8_t mode)
{
    uint8_t moduleIndex, localPin;
    if (!mapPin(pin, moduleIndex, localPin))
        return;
    if (_modules[moduleIndex])
        _modules[moduleIndex]->pinMode(localPin, mode);
}

void KTS1622_IO_Expander::digitalWrite(uint8_t pin, uint8_t value)
{
    uint8_t moduleIndex, localPin;
    if (!mapPin(pin, moduleIndex, localPin))
        return;
    if (_modules[moduleIndex])
        _modules[moduleIndex]->digitalWrite(localPin, value);
}

int KTS1622_IO_Expander::digitalRead(uint8_t pin)
{
    uint8_t moduleIndex, localPin;
    if (!mapPin(pin, moduleIndex, localPin))
        return LOW;
    if (_modules[moduleIndex])
        return _modules[moduleIndex]->digitalRead(localPin);
    return LOW;
}

void KTS1622_IO_Expander::setInterruptEnabled(uint8_t pin, bool enabled)
{
    uint8_t moduleIndex, localPin;
    if (!mapPin(pin, moduleIndex, localPin))
        return;
    if (_modules[moduleIndex])
        _modules[moduleIndex]->setInterruptEnabled(localPin, enabled);
}

void KTS1622_IO_Expander::setInterruptEdge(uint8_t pin, KTS1622_InterruptEdge edge)
{
    uint8_t moduleIndex, localPin;
    if (!mapPin(pin, moduleIndex, localPin))
        return;
    if (_modules[moduleIndex])
        _modules[moduleIndex]->setInterruptEdge(localPin, edge);
}

void KTS1622_IO_Expander::clearInterrupt(uint8_t pin)
{
    uint8_t moduleIndex, localPin;
    if (!mapPin(pin, moduleIndex, localPin))
        return;
    if (_modules[moduleIndex])
        _modules[moduleIndex]->clearInterrupt(localPin);
}

uint16_t KTS1622_IO_Expander::moduleInterruptStatus(uint8_t moduleIndex) const
{
    if (moduleIndex >= _numModules || !_modules[moduleIndex])
        return 0;
    return _modules[moduleIndex]->getInterruptStatus();
}
