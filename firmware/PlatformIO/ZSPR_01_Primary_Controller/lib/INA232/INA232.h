#ifndef _INA232_H_
#define _INA232_H_

#include <Arduino.h>
#include <Wire.h>

// I2C address, selected by the ADDR (A0) pin strap (TI INA23x family: A0 tied
// to GND or VS, A1 assumed tied GND on this board -- see datasheet Table 2 if
// a board revision ever straps A1 differently).
enum INA232_Address : uint8_t {
    INA232_ADDR_A0_GND = 0x40,
    INA232_ADDR_A0_VS  = 0x41,
};

enum INA232_Alert : uint16_t {
    INA232_ALERT_SHUNT_OVER       = 1U << 15,
    INA232_ALERT_SHUNT_UNDER      = 1U << 14,
    INA232_ALERT_BUS_OVER         = 1U << 13,
    INA232_ALERT_BUS_UNDER        = 1U << 12,
    INA232_ALERT_POWER_OVER       = 1U << 11,
    INA232_ALERT_CONVERSION_READY = 1U << 10,
    INA232_ALERT_OVERFLOW         = 1U << 9,
};

// Driver for the Texas Instruments INA232 I2C bus-voltage/shunt-current/power
// monitor. Register map matches the INA226 family. Caller is responsible for
// calling wirePort.begin(...) before begin().
class INA232 {
public:
    explicit INA232(uint8_t i2cAddress = INA232_ADDR_A0_GND, TwoWire &wirePort = Wire);

    // Probes the device (reads back the manufacturer ID register). Returns
    // true if it acknowledged and identified correctly.
    bool begin();

    // Programs the calibration register from the shunt resistor value and
    // the largest current you expect to measure -- sets the current/power
    // LSBs used to scale readCurrent_mA()/readPower_mW(). Call once after
    // begin().
    void setCalibration(float shuntOhms, float maxExpectedAmps);

    float readBusVoltage_mV();   // 1.25 mV/bit, unsigned
    float readShuntVoltage_uV(); // 2.5 uV/bit, signed
    float readCurrent_mA();      // requires setCalibration() first
    float readPower_mW();        // requires setCalibration() first

    // Configures the ALERT pin. Multiple alert sources may be ORed together.
    // ALERT is active-low by default and remains asserted until the status
    // register is read when latchAlert is true.
    void configureAlert(uint16_t sources, bool activeHigh = false, bool latchAlert = false);
    void setAlertLimitRaw(uint16_t value);
    void setShuntAlertLimit_uV(float limit_uV);
    void setBusAlertLimit_mV(float limit_mV);
    void setPowerAlertLimit_mW(float limit_mW); // requires setCalibration() first
    uint16_t readAlertStatus();

    // --- Low-level register access ---
    uint16_t readRegister(uint8_t reg);
    void     writeRegister(uint8_t reg, uint16_t value);

private:
    TwoWire *_wire;
    uint8_t  _address;
    float    _currentLSB_A = 0.0f; // amps per bit of the CURRENT register
};

#endif // _INA232_H_