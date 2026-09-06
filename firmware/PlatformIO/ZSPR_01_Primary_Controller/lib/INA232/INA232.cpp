#include "INA232.h"

namespace {
    constexpr uint8_t REG_CONFIG          = 0x00;
    constexpr uint8_t REG_SHUNT_VOLTAGE   = 0x01;
    constexpr uint8_t REG_BUS_VOLTAGE     = 0x02;
    constexpr uint8_t REG_POWER           = 0x03;
    constexpr uint8_t REG_CURRENT         = 0x04;
    constexpr uint8_t REG_CALIBRATION     = 0x05;
    constexpr uint8_t REG_MASK_ENABLE     = 0x06;
    constexpr uint8_t REG_ALERT_LIMIT     = 0x07;
    constexpr uint8_t REG_MANUFACTURER_ID = 0xFE;

    constexpr uint16_t MANUFACTURER_ID_TI = 0x5449; // "TI"
    constexpr uint16_t CONFIG_RESET       = 0x8000;

    constexpr float BUS_VOLTAGE_LSB_MV   = 1.25f;
    constexpr float SHUNT_VOLTAGE_LSB_UV = 2.5f;
    constexpr uint16_t ALERT_ACTIVE_HIGH = 1U << 1;
    constexpr uint16_t ALERT_LATCH       = 1U;
}

INA232::INA232(uint8_t i2cAddress, TwoWire &wirePort)
    : _wire(&wirePort), _address(i2cAddress) {}

bool INA232::begin() {
    writeRegister(REG_CONFIG, CONFIG_RESET);
    delay(1);
    return readRegister(REG_MANUFACTURER_ID) == MANUFACTURER_ID_TI;
}

void INA232::setCalibration(float shuntOhms, float maxExpectedAmps) {
    _currentLSB_A = maxExpectedAmps / 32768.0f;
    uint16_t cal = (uint16_t)(0.00512f / (_currentLSB_A * shuntOhms));
    writeRegister(REG_CALIBRATION, cal);
}

float INA232::readBusVoltage_mV() {
    return (float)readRegister(REG_BUS_VOLTAGE) * BUS_VOLTAGE_LSB_MV;
}

float INA232::readShuntVoltage_uV() {
    return (float)(int16_t)readRegister(REG_SHUNT_VOLTAGE) * SHUNT_VOLTAGE_LSB_UV;
}

float INA232::readCurrent_mA() {
    return (float)(int16_t)readRegister(REG_CURRENT) * _currentLSB_A * 1000.0f;
}

float INA232::readPower_mW() {
    // Power register LSB is fixed at 25x the current LSB (unsigned).
    return (float)readRegister(REG_POWER) * _currentLSB_A * 25.0f * 1000.0f;
}

void INA232::configureAlert(uint16_t sources, bool activeHigh, bool latchAlert) {
    uint16_t value = sources & 0xFE00U;
    if (activeHigh) value |= ALERT_ACTIVE_HIGH;
    if (latchAlert) value |= ALERT_LATCH;
    writeRegister(REG_MASK_ENABLE, value);
}

void INA232::setAlertLimitRaw(uint16_t value) {
    writeRegister(REG_ALERT_LIMIT, value);
}

void INA232::setShuntAlertLimit_uV(float limit_uV) {
    setAlertLimitRaw((uint16_t)(limit_uV / SHUNT_VOLTAGE_LSB_UV));
}

void INA232::setBusAlertLimit_mV(float limit_mV) {
    setAlertLimitRaw((uint16_t)(limit_mV / BUS_VOLTAGE_LSB_MV));
}

void INA232::setPowerAlertLimit_mW(float limit_mW) {
    setAlertLimitRaw((uint16_t)(limit_mW / (_currentLSB_A * 25.0f * 1000.0f)));
}

uint16_t INA232::readAlertStatus() {
    return readRegister(REG_MASK_ENABLE);
}

uint16_t INA232::readRegister(uint8_t reg) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->endTransmission(false);
    _wire->requestFrom((int)_address, 2);
    if (_wire->available() < 2) return 0;
    uint16_t hi = _wire->read();
    uint16_t lo = _wire->read();
    return (uint16_t)((hi << 8) | lo);
}

void INA232::writeRegister(uint8_t reg, uint16_t value) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write((uint8_t)(value >> 8));
    _wire->write((uint8_t)(value & 0xFF));
    _wire->endTransmission();
}
