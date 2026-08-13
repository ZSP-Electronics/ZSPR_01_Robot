
#include "BQ25883.h"

BQ25883::BQ25883(){}

#ifdef HAL_I2C_MODULE_ENABLED
bool BQ25883::begin(Board_I2C_Handle handle){
    _handle = handle;
    // check PN & DEV

    // uint8_t regCheck = this->readPartInfoReg();
    if(this->readPartInfoReg() != BQ25883_PARTINFO){
        if(!Board_I2C_IsDeviceReady(_handle, BQ25883_I2C_ADDRESS))
            return false;
    }
    return true;
}
#else
bool BQ25883::begin(){
    return true;
}
#endif


// --- Register accessors and helpers ---
uint8_t BQ25883::readCellVoltageLimitReg(){
    this->rawCellVoltageLimitReg = this->read8(BQ25883_REG_VOLTAGE_LIMIT);
    return this->rawCellVoltageLimitReg;
}

float BQ25883::getCellVoltageLimit(){
    float cellVoltageLimit = 3.4;
    cellVoltageLimit = cellVoltageLimit + (0.005 * this->rawCellVoltageLimitReg);
    if(cellVoltageLimit > 4.6){
        cellVoltageLimit = 4.6;
    }
    return cellVoltageLimit;
}

uint8_t BQ25883::readChargeCurrentLimitReg(){
    this->rawChargeCurrentLimitReg = this->read8(BQ25883_REG_CHARGE_CURRENT_LIMIT);
    return this->rawChargeCurrentLimitReg;
}

float BQ25883::getChargeCurrentLimit(){
    float chargeCurrentLimit = 0.0;
    chargeCurrentLimit = chargeCurrentLimit + (1.600 * ((0b00100000 & this->rawChargeCurrentLimitReg) >> 5));
    chargeCurrentLimit = chargeCurrentLimit + (0.800 * ((0b00010000 & this->rawChargeCurrentLimitReg) >> 4));
    chargeCurrentLimit = chargeCurrentLimit + (0.400 * ((0b00001000 & this->rawChargeCurrentLimitReg) >> 3));
    chargeCurrentLimit = chargeCurrentLimit + (0.200 * ((0b00000100 & this->rawChargeCurrentLimitReg) >> 2));
    chargeCurrentLimit = chargeCurrentLimit + (0.100 * ((0b00000010 & this->rawChargeCurrentLimitReg) >> 1));
    chargeCurrentLimit = chargeCurrentLimit + (0.050 * ((0b00000001 & this->rawChargeCurrentLimitReg) >> 0));
    if(chargeCurrentLimit < 0.1){
        chargeCurrentLimit = 0.1;
    }
    if(chargeCurrentLimit > 2.2){
        chargeCurrentLimit = 2.2;
    }
    return chargeCurrentLimit;
}

bool BQ25883::getHIZMode(){
    return ((0b10000000 & this->rawChargeCurrentLimitReg) >> 7);
}

bool BQ25883::getILIMPinFunction(){
    return ((0b01000000 & this->rawChargeCurrentLimitReg) >> 6);
}

uint8_t BQ25883::readInputVoltageLimitReg(){
    this->rawInputVoltageLimitReg = this->read8(BQ25883_REG_INPUT_VOLTAGE_LIMIT);
    return this->rawInputVoltageLimitReg;
}

float BQ25883::getInputVoltageLimit(){
    float inputVoltageLimit = 3.9;
    inputVoltageLimit = inputVoltageLimit + (1.600 * ((0b00010000 & this->rawInputVoltageLimitReg) >> 4));
    inputVoltageLimit = inputVoltageLimit + (0.800 * ((0b00001000 & this->rawInputVoltageLimitReg) >> 3));
    inputVoltageLimit = inputVoltageLimit + (0.400 * ((0b00000100 & this->rawInputVoltageLimitReg) >> 2));
    inputVoltageLimit = inputVoltageLimit + (0.200 * ((0b00000010 & this->rawInputVoltageLimitReg) >> 1));
    inputVoltageLimit = inputVoltageLimit + (0.100 * ((0b00000001 & this->rawInputVoltageLimitReg) >> 0));
    if(inputVoltageLimit > 5.5){
        inputVoltageLimit = 5.5;
    }
    return inputVoltageLimit;
}

bool BQ25883::getEN_VINDPM_RST(){
    return ((0b10000000 & this->rawInputVoltageLimitReg) >> 7);
}

bool BQ25883::getEN_BAT_DISCHG(){
    return ((0b01000000 & this->rawInputVoltageLimitReg) >> 6);
}

bool BQ25883::getPFM_OOA_DIS(){
    return ((0b00100000 & this->rawInputVoltageLimitReg) >> 5);
}

uint8_t BQ25883::readInputCurrentLimitReg(){
    this->rawInputCurrentLimitReg = this->read8(BQ25883_REG_INPUT_CURRENT_LIMIT);
    return this->rawInputCurrentLimitReg;
}

float BQ25883::getInputCurrentLimit(){
    float inputCurrentLimit = 0.5;
    inputCurrentLimit = inputCurrentLimit + (1.600 * ((0b00010000 & this->rawInputCurrentLimitReg) >> 4));
    inputCurrentLimit = inputCurrentLimit + (0.800 * ((0b00001000 & this->rawInputCurrentLimitReg) >> 3));
    inputCurrentLimit = inputCurrentLimit + (0.400 * ((0b00000100 & this->rawInputCurrentLimitReg) >> 2));
    inputCurrentLimit = inputCurrentLimit + (0.200 * ((0b00000010 & this->rawInputCurrentLimitReg) >> 1));
    inputCurrentLimit = inputCurrentLimit + (0.100 * ((0b00000001 & this->rawInputCurrentLimitReg) >> 0));
    if(inputCurrentLimit > 3.3){
        inputCurrentLimit = 3.3;
    }
    return inputCurrentLimit;
}

bool BQ25883::getFORCE_ICO(){
    return ((0b10000000 & this->rawInputCurrentLimitReg) >> 7);
}

bool BQ25883::getFORCE_INDET(){
    return ((0b01000000 & this->rawInputCurrentLimitReg) >> 6);
}

bool BQ25883::getEN_ICO(){
    return ((0b00100000 & this->rawInputCurrentLimitReg) >> 5);
}

uint8_t BQ25883::readPreTermCurrentLimitReg(){
    this->rawPreTermCurrentLimitReg = this->read8(BQ25883_REG_PRE_TERM_CURRENT_LIMIT);
    return this->rawPreTermCurrentLimitReg;
}

float BQ25883::getPrechargeCurrentLimit(){
    float prechargeCurrentLimit = 0.05;
    prechargeCurrentLimit = prechargeCurrentLimit + (0.400 * ((0b10000000 & this->rawPreTermCurrentLimitReg) >> 7));
    prechargeCurrentLimit = prechargeCurrentLimit + (0.200 * ((0b01000000 & this->rawPreTermCurrentLimitReg) >> 6));
    prechargeCurrentLimit = prechargeCurrentLimit + (0.100 * ((0b00100000 & this->rawPreTermCurrentLimitReg) >> 5));
    prechargeCurrentLimit = prechargeCurrentLimit + (0.050 * ((0b00010000 & this->rawPreTermCurrentLimitReg) >> 4));
    if(prechargeCurrentLimit > 0.8){
        prechargeCurrentLimit = 0.8;
    }
    return prechargeCurrentLimit;
}

float BQ25883::getTerminationCurrentLimit(){
    float terminationCurrentLimit = 0.05;
    terminationCurrentLimit = terminationCurrentLimit + (0.400 * ((0b00001000 & this->rawPreTermCurrentLimitReg) >> 3));
    terminationCurrentLimit = terminationCurrentLimit + (0.200 * ((0b00000100 & this->rawPreTermCurrentLimitReg) >> 2));
    terminationCurrentLimit = terminationCurrentLimit + (0.100 * ((0b00000010 & this->rawPreTermCurrentLimitReg) >> 1));
    terminationCurrentLimit = terminationCurrentLimit + (0.050 * ((0b00000001 & this->rawPreTermCurrentLimitReg) >> 0));
    if(terminationCurrentLimit > 0.8){
        terminationCurrentLimit = 0.8;
    }
    return terminationCurrentLimit;
}

uint32_t BQ25883::readChargeControlSettingsReg(){
    this->rawChargeControlSettingsReg = this->read32(BQ25883_REG_CHARGER_CONTROL_1);
    return this->rawChargeControlSettingsReg;
}

bool BQ25883::getEN_TERM(){
    return ((0x00000080 & this->rawChargeControlSettingsReg) >> 7);
}

bool BQ25883::getSTAT_DIS(){
    return ((0x00000040 & this->rawChargeControlSettingsReg) >> 6);
}

uint8_t BQ25883::getWATCHDOG(){
    uint8_t watchdog = ((0x00000030 & this->rawChargeControlSettingsReg) >> 4);
    switch(watchdog){
        case 0b00: watchdog = 0; break;
        case 0b01: watchdog = 40; break;
        case 0b10: watchdog = 80; break;
        case 0b11: watchdog = 160; break;
        default: watchdog = 255; break;
    }
    return watchdog;
}

bool BQ25883::getEN_TIMER(){
    return ((0x00000008 & this->rawChargeControlSettingsReg) >> 3);
}

uint8_t BQ25883::getCHG_TIMER(){
    uint8_t chgTimer = ((0x00000006 & this->rawChargeControlSettingsReg) >> 1);
    switch(chgTimer){
        case 0b00: chgTimer = 5; break;
        case 0b01: chgTimer = 8; break;
        case 0b10: chgTimer = 12; break;
        case 0b11: chgTimer = 20; break;
        default: chgTimer = 255; break;
    }
    return chgTimer;
}

bool BQ25883::getTMR2X_EN(){
    return ((0x00000001 & this->rawChargeControlSettingsReg) >> 0);
}

bool BQ25883::getAUTO_INDET_EN(){
    return ((0x00004000 & this->rawChargeControlSettingsReg) >> 14);
}

uint8_t BQ25883::getT_REG_THRESH(){
    uint8_t tRegThresh = ((0x00003000 & this->rawChargeControlSettingsReg) >> 12);
    switch(tRegThresh){
        case 0b00: tRegThresh = 60; break;
        case 0b01: tRegThresh = 80; break;
        case 0b10: tRegThresh = 100; break;
        case 0b11: tRegThresh = 120; break;
        default: tRegThresh = 255; break;
    }
    return tRegThresh;
}

bool BQ25883::getEN_CHG(){
    return ((0x00000800 & this->rawChargeControlSettingsReg) >> 11);
}

float BQ25883::getCELLLOWV_THRESH(){
    bool preChgThresh = ((0x00000400 & this->rawChargeControlSettingsReg) >> 10);
    if(preChgThresh) return 3.0; else return 2.8;
}

float BQ25883::getVCELL_RECHG_THRESH_OFF(){
    uint8_t vCellRechgThreshOffReg = ((0x00000300 & this->rawChargeControlSettingsReg) >> 8);
    float vCellRechgThreshOff  = 0.05;
    vCellRechgThreshOff  = (0.05 * vCellRechgThreshOffReg);
    return vCellRechgThreshOff;
}

bool BQ25883::getPFM_DIS(){
    return ((0x00800000 & this->rawChargeControlSettingsReg) >> 23);
}

bool BQ25883::getWD_RST(){
    return ((0x00400000 & this->rawChargeControlSettingsReg) >> 22);
}

uint8_t BQ25883::getTOPOFF_TIMER(){
    uint8_t topOffTimer = ((0x00300000 & this->rawChargeControlSettingsReg) >> 20);
    switch(topOffTimer){
        case 0b00: topOffTimer = 0; break;
        case 0b01: topOffTimer = 15; break;
        case 0b10: topOffTimer = 30; break;
        case 0b11: topOffTimer = 45; break;
        default: topOffTimer = 255; break;
    }
    return topOffTimer;
}

float BQ25883::getJEITA_VSET(){
    uint8_t jeitaVsetReg = ((0x18000000 & this->rawChargeControlSettingsReg) >> 27);
    float jeitaVset = 999.99;
    switch(jeitaVsetReg){
        case 0b00: jeitaVset = 0.0; break;
        case 0b01: jeitaVset = 8.0; break;
        case 0b10: jeitaVset = 8.3; break;
        case 0b11: jeitaVset = 111.11; break;
        default: jeitaVset = 888.88; break;
    }
    return jeitaVset;
}

uint8_t BQ25883::getJEITA_ISETH(){
    uint8_t jeitaIsetH = ((0x04000000 & this->rawChargeControlSettingsReg) >> 26);
    switch(jeitaIsetH){
        case 0: jeitaIsetH = 40; break;
        case 1: jeitaIsetH = 100; break;
        default: jeitaIsetH = 255; break;
    }
    return jeitaIsetH;
}

uint8_t BQ25883::getJEITA_ISETC(){
    uint8_t jeitaIsetC = ((0x03000000 & this->rawChargeControlSettingsReg) >> 24);
    switch(jeitaIsetC){
        case 0b00: jeitaIsetC = 0; break;
        case 0b01: jeitaIsetC = 20; break;
        case 0b10: jeitaIsetC = 40; break;
        case 0b11: jeitaIsetC = 100; break;
        default: jeitaIsetC = 255; break;
    }
    return jeitaIsetC;
}

uint8_t BQ25883::readIcoCurrentLimitReg(){
    this->rawIcoCurrentLimitReg = this->read8(BQ25883_REG_ICO_CURRENT_LIMIT);
    return this->rawIcoCurrentLimitReg;
}

float BQ25883::getICOCurrentLimit(){
    float icoCurrentLimit = 0.5;
    icoCurrentLimit = icoCurrentLimit + (1.600 * ((0b00010000 & this->rawIcoCurrentLimitReg) >> 4));
    icoCurrentLimit = icoCurrentLimit + (0.800 * ((0b00001000 & this->rawIcoCurrentLimitReg) >> 3));
    icoCurrentLimit = icoCurrentLimit + (0.400 * ((0b00000100 & this->rawIcoCurrentLimitReg) >> 2));
    icoCurrentLimit = icoCurrentLimit + (0.200 * ((0b00000010 & this->rawIcoCurrentLimitReg) >> 1));
    icoCurrentLimit = icoCurrentLimit + (0.100 * ((0b00000001 & this->rawIcoCurrentLimitReg) >> 0));
    if(icoCurrentLimit > 3.3){ icoCurrentLimit = 3.3; }
    return icoCurrentLimit;
}

uint16_t BQ25883::readChargeStatusReg(){
    this->rawChargeStatusReg = this->read16(BQ25883_REG_CHARGER_STATUS_1);
    return this->rawChargeStatusReg;
}

bool BQ25883::getIINDPM_STAT(){ return ((0x0040 & this->rawChargeStatusReg) >> 6); }
bool BQ25883::getVINDPM_STAT(){ return ((0x0020 & this->rawChargeStatusReg) >> 5); }
bool BQ25883::getTREG_STAT(){ return ((0x0010 & this->rawChargeStatusReg) >> 4); }
bool BQ25883::getWD_STAT(){ return ((0x0008 & this->rawChargeStatusReg) >> 3); }
uint8_t BQ25883::getCHRG_STAT(){ return ((0x0007 & this->rawChargeStatusReg) >> 0); }
bool BQ25883::getPG_STAT(){ return ((0x8000 & this->rawChargeStatusReg) >> 15); }
uint8_t BQ25883::getVBUS_STAT(){ return ((0x7000 & this->rawChargeStatusReg) >> 12); }
uint8_t BQ25883::getICO_STAT(){ return ((0x0600 & this->rawChargeStatusReg) >> 9); }

uint8_t BQ25883::readNtcStatusReg(){ this->rawNtcStatusReg = this->read8(BQ25883_REG_NTC_STATUS); return this->rawNtcStatusReg; }
uint8_t BQ25883::getNTCStatus(){ return ((0b00000111 & this->rawNtcStatusReg) >> 0); }

uint8_t BQ25883::readFaultStatusReg(){ this->rawFaultStatusReg = this->read8(BQ25883_REG_FAULT_STATUS); return this->rawFaultStatusReg; }
bool BQ25883::getVBUS_OVP_STAT(){ return ((0b10000000 & this->rawFaultStatusReg) >> 7); }
bool BQ25883::getTSHUT_STAT(){ return ((0b01000000 & this->rawFaultStatusReg) >> 6); }
bool BQ25883::getTMR_STAT(){ return ((0b00010000 & this->rawFaultStatusReg) >> 4); }

uint16_t BQ25883::readChargeFlagsReg(){ this->rawChargeFlagsReg = this->read16(BQ25883_REG_CHARGER_FLAG_1); return this->rawChargeFlagsReg; }
bool BQ25883::getIINDPM_FLAG(){ return ((0x0040 & this->rawChargeFlagsReg) >> 6); }
bool BQ25883::getVINDPM_FLAG(){ return ((0x0020 & this->rawChargeFlagsReg) >> 5); }
bool BQ25883::getTREG_FLAG(){ return ((0x0010 & this->rawChargeFlagsReg) >> 4); }
bool BQ25883::getWD_FLAG(){ return ((0x0008 & this->rawChargeFlagsReg) >> 3); }
bool BQ25883::getCHRG_FLAG(){ return ((0x0001 & this->rawChargeFlagsReg) >> 0); }
bool BQ25883::getPG_FLAG(){ return ((0x8000 & this->rawChargeFlagsReg) >> 15); }
bool BQ25883::getVBUS_FLAG(){ return ((0x1000 & this->rawChargeFlagsReg) >> 12); }
bool BQ25883::getTS_FLAG(){ return ((0x0400 & this->rawChargeFlagsReg) >> 10); }
bool BQ25883::getICO_FLAG(){ return ((0x0200 & this->rawChargeFlagsReg) >> 9); }

uint8_t BQ25883::readFaultFlagsReg(){ this->rawFaultFlagsReg = this->read8(BQ25883_REG_FAULT_FLAG); return this->rawFaultFlagsReg; }
bool BQ25883::getVBUS_OVP_FLAG(){ return ((0b10000000 & this->rawFaultFlagsReg) >> 7); }
bool BQ25883::getTSHUT_FLAG(){ return ((0b01000000 & this->rawFaultFlagsReg) >> 6); }
bool BQ25883::getTMR_FLAG(){ return ((0b00010000 & this->rawFaultFlagsReg) >> 4); }

uint16_t BQ25883::readChargerIntMaskReg(){ this->rawChargerIntMaskReg = this->read16(BQ25883_REG_CHARGER_MASK_1); return this->rawChargerIntMaskReg; }
bool BQ25883::getADC_DONE_MASK(){ return ((0x0080 & this->rawChargerIntMaskReg) >> 7); }
bool BQ25883::getIINDPM_MASK(){ return ((0x0040 & this->rawChargerIntMaskReg) >> 6); }
bool BQ25883::getVINDPM_MASK(){ return ((0x0020 & this->rawChargerIntMaskReg) >> 5); }
bool BQ25883::getT_REG_MASK(){ return ((0x0010 & this->rawChargerIntMaskReg) >> 4); }
bool BQ25883::getWD_MASK(){ return ((0x0008 & this->rawChargerIntMaskReg) >> 3); }
bool BQ25883::getCHRG_MASK(){ return ((0x0001 & this->rawChargerIntMaskReg) >> 0); }
bool BQ25883::getPG_MASK(){ return ((0x8000 & this->rawChargerIntMaskReg) >> 15); }
bool BQ25883::getVBUS_MASK(){ return ((0x1000 & this->rawChargerIntMaskReg) >> 12); }
bool BQ25883::getTS_MASK(){ return ((0x0400 & this->rawChargerIntMaskReg) >> 10); }
bool BQ25883::getICO_MASK(){ return ((0x0200 & this->rawChargerIntMaskReg) >> 9); }

uint8_t BQ25883::readFaultIntMaskReg(){ this->rawFaultIntMaskReg = this->read8(BQ25883_REG_FAULT_MASK); return this->rawFaultIntMaskReg; }
bool BQ25883::getVBUS_OVP_MASK(){ return ((0b10000000 & this->rawFaultIntMaskReg) >> 7); }
bool BQ25883::getTSHUT_MASK(){ return ((0b01000000 & this->rawFaultIntMaskReg) >> 6); }
bool BQ25883::getTMR_MASK(){ return ((0b00010000 & this->rawFaultIntMaskReg) >> 4); }
bool BQ25883::getSNS_SHORT_MASK(){ return ((0b00001000 & this->rawFaultIntMaskReg) >> 3); }

uint8_t BQ25883::readADCControlSettingsReg(){ this->rawADCControlSettingsReg = this->read8(BQ25883_REG_ADC_CONTROL); return this->rawADCControlSettingsReg; }
bool BQ25883::getADC_EN(){ return ((0b10000000 & this->rawADCControlSettingsReg) >> 7); }
bool BQ25883::getADC_ONE_SHOT(){ return ((0b01000000 & this->rawADCControlSettingsReg) >> 6); }
uint8_t BQ25883::getADC_SAMPLE_SPEED(){ return ((0b00110000 & this->rawADCControlSettingsReg) >> 4); }

uint8_t BQ25883::readADCFuncDisSettingsReg(){ this->rawADCFuncDisSettingsReg = this->read8(BQ25883_REG_ADC_FUNCTION_DISABLE); return this->rawADCFuncDisSettingsReg; }
bool BQ25883::getIBUS_ADC_DIS(){ return ((0b10000000 & this->rawADCFuncDisSettingsReg) >> 7); }
bool BQ25883::getICHG_ADC_DIS(){ return ((0b01000000 & this->rawADCFuncDisSettingsReg) >> 6); }
bool BQ25883::getVBUS_ADC_DIS(){ return ((0b00100000 & this->rawADCFuncDisSettingsReg) >> 5); }
bool BQ25883::getVBAT_ADC_DIS(){ return ((0b00010000 & this->rawADCFuncDisSettingsReg) >> 4); }
bool BQ25883::getTS_ADC_DIS(){ return ((0b00000100 & this->rawADCFuncDisSettingsReg) >> 2); }
bool BQ25883::getVCELL_ADC_DIS(){ return ((0b00000010 & this->rawADCFuncDisSettingsReg) >> 1); }
bool BQ25883::getTDIE_ADC_DIS(){ return ((0b00000001 & this->rawADCFuncDisSettingsReg) >> 0); }

int16_t BQ25883::readADCIbusReg(){ this->rawADCIbusReg = this->readADC(BQ25883_REG_IBUS_ADC_1); return this->rawADCIbusReg; }
float BQ25883::getADC_IBUS(){ return float(this->rawADCIbusReg)/1000.0; }

int16_t BQ25883::readADCIchgReg(){ this->rawADCIchgReg = this->readADC(BQ25883_REG_ICHG_ADC_1); return this->rawADCIchgReg; }
float BQ25883::getADC_ICHG(){ return float(this->rawADCIchgReg)/1000.0; }

int16_t BQ25883::readADCVbusReg(){ this->rawADCVbusReg = this->readADC(BQ25883_REG_VBUS_ADC_1); return this->rawADCVbusReg; }
float BQ25883::getADC_VBUS(){ return float(this->rawADCVbusReg)/1000.0; }

int16_t BQ25883::readADCVbatReg(){ this->rawADCVbatReg = this->readADC(BQ25883_REG_VBAT_ADC_1); return this->rawADCVbatReg; }
float BQ25883::getADC_VBAT(){ return float(this->rawADCVbatReg)/1000.0; }

int16_t BQ25883::readADCVSysReg(){ this->rawADCVSysReg = this->readADC(BQ25883_REG_VSYS_ADC_1); return this->rawADCVSysReg; }
float BQ25883::getADC_VSYS(){ return float(this->rawADCVSysReg)/1000.0; }

int16_t BQ25883::readADCVCellTopReg(){ this->rawADCVCellTopReg = this->readADC(BQ25883_REG_VSYS_ADC_1); return this->rawADCVCellTopReg; }
float BQ25883::getADC_VCELLTOP(){ return float(this->rawADCVCellTopReg)/1000.0; }

uint16_t BQ25883::readADCTsReg(){ this->rawADCTsReg = this->read16(BQ25883_REG_TS_ADC_1); return this->rawADCTsReg; }
float BQ25883::getADC_TS(){
    float adcTs = 0.0;
    adcTs = adcTs + (50.000 * ((0x0002 & this->rawADCTsReg) >> 1));
    adcTs = adcTs + (25.000 * ((0x0001 & this->rawADCTsReg) >> 0));
    adcTs = adcTs + (12.500 * ((0x8000 & this->rawADCTsReg) >> 15));
    adcTs = adcTs + (6.250 * ((0x4000 & this->rawADCTsReg) >> 14));
    adcTs = adcTs + (3.125 * ((0x2000 & this->rawADCTsReg) >> 13));
    adcTs = adcTs + (1.563 * ((0x1000 & this->rawADCTsReg) >> 12));
    adcTs = adcTs + (0.781 * ((0x0800 & this->rawADCTsReg) >> 11));
    adcTs = adcTs + (0.391 * ((0x0400 & this->rawADCTsReg) >> 10));
    adcTs = adcTs + (0.195 * ((0x0200 & this->rawADCTsReg) >> 9));
    adcTs = adcTs + (0.098 * ((0x0100 & this->rawADCTsReg) >> 8));
    if(adcTs > 94.9){ adcTs = 94.9; }
    return adcTs;
}

uint16_t BQ25883::readADCTDieReg(){ this->rawADCTDieReg = this->read16(BQ25883_REG_TDIE_ADC_1); return this->rawADCTDieReg; }
float BQ25883::getADC_TDIE(){
    float adcTDie = 0.0;
    adcTDie = adcTDie + (128.0 * ((0x0001 & this->rawADCTDieReg) >> 0));
    adcTDie = adcTDie + (64.0 * ((0x8000 & this->rawADCTDieReg) >> 15));
    adcTDie = adcTDie + (32.0 * ((0x4000 & this->rawADCTDieReg) >> 14));
    adcTDie = adcTDie + (16.0 * ((0x2000 & this->rawADCTDieReg) >> 13));
    adcTDie = adcTDie + (8.0 * ((0x1000 & this->rawADCTDieReg) >> 12));
    adcTDie = adcTDie + (4.0 * ((0x0800 & this->rawADCTDieReg) >> 11));
    adcTDie = adcTDie + (2.0 * ((0x0400 & this->rawADCTDieReg) >> 10));
    adcTDie = adcTDie + (1.0 * ((0x0200 & this->rawADCTDieReg) >> 9));
    adcTDie = adcTDie + (0.5 * ((0x0100 & this->rawADCTDieReg) >> 8));
    if(adcTDie > 128.0){ adcTDie = 128.0; }
    return adcTDie;
}

int16_t BQ25883::readADCVCellBotReg(){ this->rawADCVCellBotReg = this->readADC(BQ25883_REG_VSYS_ADC_1); return this->rawADCVCellBotReg; }
float BQ25883::getADC_VCELLBOT(){ return float(this->rawADCVCellBotReg)/1000.0; }

uint8_t BQ25883::readPartInfoReg(){ this->rawPartInfoReg = this->read8(BQ25883_REG_PART_INFORMATION); return this->rawPartInfoReg; }
bool BQ25883::getREG_RST(){ return ((0b10000000 & this->rawPartInfoReg) >> 7); }
uint8_t BQ25883::getPartNumber(){ return ((0b01111000 & this->rawPartInfoReg) >> 3); }
uint8_t BQ25883::getDevRev(){ return ((0b00000111 & this->rawPartInfoReg) >> 0); }

// uint16_t BQ25883::readCellBalContSettingsReg(){ this->rawCellBalContSettingsReg = this->read16(BQ25883_REG_CELL_BALANCING_CONTROL_1); return this->rawCellBalContSettingsReg; }
// float BQ25883::getVDIFF_END_OFFSET(){ float vDiffEndOffset = 0.030; vDiffEndOffset = vDiffEndOffset + (0.01 * ((0x00E0 & this->rawCellBalContSettingsReg) >> 5)); return vDiffEndOffset; }
// float BQ25883::getTCB_QUAL_INTERVAL(){ if(((0x0010 & this->rawCellBalContSettingsReg) >> 4)) return 240.0; else return 120.0; }
// float BQ25883::getTCB_ACTIVE(){ uint16_t v = ((0x000C & this->rawCellBalContSettingsReg) >> 2); switch(v){ case 0b00: return 4.0; case 0b01: return 32.0; case 0b10: return 120.0; case 0b11: return 240.0; default: return 999.99; } }
// float BQ25883::getTSETTLE(){ uint16_t v = ((0x0003 & this->rawCellBalContSettingsReg) >> 0); switch(v){ case 0b00: return 0.01; case 0b01: return 0.10; case 0b10: return 1.0; case 0b11: return 2.0; default: return 999.99; } }
// float BQ25883::getVQUAL_TH(){ float vQualTh = 0.04; uint16_t v = ((0xF000 & this->rawCellBalContSettingsReg) >> 12); if(v != 0x000F) vQualTh += (0.01 * v); else vQualTh = 0.0; return vQualTh; }
// float BQ25883::getVDIFF_START(){ float vDiffStart = 0.04; uint16_t v = ((0x0F00 & this->rawCellBalContSettingsReg) >> 8); vDiffStart += (0.01 * v); return vDiffStart; }

// // uint8_t BQ25883::readCellBalStatReg(){ this->rawCellBalStatReg = this->read8(BQ25883_REG_CELL_BALANCING_STAT_CONT); return this->rawCellBalStatReg; }
// bool BQ25883::getCB_CHG_DIS(){ return ((0b10000000 & this->rawCellBalStatReg) >> 7); }
// bool BQ25883::getCB_AUTO_EN(){ return ((0b01000000 & this->rawCellBalStatReg) >> 6); }
// bool BQ25883::getCB_STAT(){ return ((0b00100000 & this->rawCellBalStatReg) >> 5); }
// bool BQ25883::getHS_CV_STAT(){ return ((0b00010000 & this->rawCellBalStatReg) >> 4); }
// bool BQ25883::getLS_CV_STAT(){ return ((0b00001000 & this->rawCellBalStatReg) >> 3); }
// bool BQ25883::getHS_OV_STAT(){ return ((0b00000100 & this->rawCellBalStatReg) >> 2); }
// bool BQ25883::getLS_OV_STAT(){ return ((0b00000010 & this->rawCellBalStatReg) >> 1); }
// bool BQ25883::getCB_OC_STAT(){ return ((0b00000001 & this->rawCellBalStatReg) >> 0); }

// // uint8_t BQ25883::readCellBalFlagsReg(){ this->rawCellBalFlagsReg = this->read8(BQ25883_REG_CELL_BALANCING_FLAG); return this->rawCellBalFlagsReg; }
// bool BQ25883::getQCBH_EN(){ return ((0b10000000 & this->rawCellBalFlagsReg) >> 7); }
// bool BQ25883::getQCBL_EN(){ return ((0b01000000 & this->rawCellBalFlagsReg) >> 6); }
// bool BQ25883::getCB_FLAG(){ return ((0b00100000 & this->rawCellBalFlagsReg) >> 5); }
// bool BQ25883::getHS_CV_FLAG(){ return ((0b00010000 & this->rawCellBalFlagsReg) >> 4); }
// bool BQ25883::getLS_CV_FLAG(){ return ((0b00001000 & this->rawCellBalFlagsReg) >> 3); }
// bool BQ25883::getHS_OV_FLAG(){ return ((0b00000100 & this->rawCellBalFlagsReg) >> 2); }
// bool BQ25883::getLS_OV_FLAG(){ return ((0b00000010 & this->rawCellBalFlagsReg) >> 1); }
// bool BQ25883::getCB_OC_FLAG(){ return ((0b00000001 & this->rawCellBalFlagsReg) >> 0); }

// // uint8_t BQ25883::readCellBalIntMaskReg(){ this->rawCellBalIntMaskReg = this->read8(BQ25883_REG_CELL_BALANCING_MASK); return this->rawCellBalIntMaskReg; }
// bool BQ25883::getCB_MASK(){ return ((0b00100000 & this->rawCellBalIntMaskReg) >> 5); }
// bool BQ25883::getHS_CV_MASK(){ return ((0b00010000 & this->rawCellBalIntMaskReg) >> 4); }
// bool BQ25883::getLS_CV_MASK(){ return ((0b00001000 & this->rawCellBalIntMaskReg) >> 3); }
// bool BQ25883::getHS_OV_MASK(){ return ((0b00000100 & this->rawCellBalIntMaskReg) >> 2); }
// bool BQ25883::getLS_OV_MASK(){ return ((0b00000010 & this->rawCellBalIntMaskReg) >> 1); }
// bool BQ25883::getCB_OC_MASK(){ return ((0b00000001 & this->rawCellBalIntMaskReg) >> 0); }

void BQ25883::pollAllRegs(){
    this->readCellVoltageLimitReg();
    this->readChargeCurrentLimitReg();
    this->readInputVoltageLimitReg();
    this->readInputCurrentLimitReg();
    this->readPreTermCurrentLimitReg();
    this->readChargeControlSettingsReg();
    this->readIcoCurrentLimitReg();
    this->readChargeStatusReg();
    this->readNtcStatusReg();
    this->readFaultStatusReg();
    this->readChargeFlagsReg();
    this->readFaultFlagsReg();
    this->readChargerIntMaskReg();
    this->readFaultIntMaskReg();
    this->readADCControlSettingsReg();
    this->readADCFuncDisSettingsReg();
    this->readADCIbusReg();
    this->readADCIchgReg();
    this->readADCVbusReg();
    this->readADCVbatReg();
    this->readADCVSysReg();
    this->readADCVCellTopReg();
    this->readADCTsReg();
    this->readADCTDieReg();
    this->readADCVCellBotReg();
    this->readPartInfoReg();
    // this->readCellBalContSettingsReg();
    // this->readCellBalStatReg();
    // this->readCellBalFlagsReg();
    // this->readCellBalIntMaskReg();
}

void BQ25883::setCellVoltageLimit(float voltLimit){
    if(voltLimit < 3.4) voltLimit = 3.4; else if(voltLimit > 4.6) voltLimit = 4.6;
    uint8_t cellVoltageLimit = lround(((voltLimit - 3.4)*1000.0)/5.0);
    this->write8(BQ25883_REG_VOLTAGE_LIMIT, cellVoltageLimit);
}

void BQ25883::setChargeCurrentLimit(float chgCurrentLimit){
    uint8_t regValue = this->read8(BQ25883_REG_CHARGE_CURRENT_LIMIT);
    regValue = (regValue & 0b11000000);
    if(chgCurrentLimit < 0.1) chgCurrentLimit = 0.1; else if(chgCurrentLimit > 2.2) chgCurrentLimit = 2.2;
    uint8_t chargeCurrentLimit = lround((chgCurrentLimit*1000.0)/50.0);
    chargeCurrentLimit = (chargeCurrentLimit & 0b00111111);
    regValue = (regValue | chargeCurrentLimit);
    this->write8(BQ25883_REG_CHARGE_CURRENT_LIMIT, regValue);
}

void BQ25883::setHIZMode(bool enable){ uint8_t regValue = this->read8(BQ25883_REG_CHARGE_CURRENT_LIMIT); regValue = (regValue & 0b01111111); if(enable) regValue = (regValue | 0b10000000); this->write8(BQ25883_REG_CHARGE_CURRENT_LIMIT, regValue); }
void BQ25883::setILIMPinFunction(bool enable){ uint8_t regValue = this->read8(BQ25883_REG_CHARGE_CURRENT_LIMIT); regValue = (regValue & 0b10111111); if(enable) regValue = (regValue | 0b01000000); this->write8(BQ25883_REG_CHARGE_CURRENT_LIMIT, regValue); }

void BQ25883::setInputVoltageLimit(float inputVoltLimit){ uint8_t regValue = this->read8(BQ25883_REG_INPUT_VOLTAGE_LIMIT); regValue = (regValue & 0b11100000); if(inputVoltLimit < 3.9) inputVoltLimit = 3.9; else if(inputVoltLimit > 5.5) inputVoltLimit = 5.5; uint8_t inputVoltageLimit = lround(((inputVoltLimit - 3.9)*1000.0)/100.0); inputVoltageLimit = (inputVoltageLimit & 0b00011111); regValue = (regValue | inputVoltageLimit); this->write8(BQ25883_REG_INPUT_VOLTAGE_LIMIT, regValue); }
void BQ25883::setEN_VINDPM_RST(bool enable){ uint8_t regValue = this->read8(BQ25883_REG_INPUT_VOLTAGE_LIMIT); regValue = (regValue & 0b01111111); if(enable) regValue = (regValue | 0b10000000); this->write8(BQ25883_REG_INPUT_VOLTAGE_LIMIT, regValue); }
void BQ25883::setEN_BAT_DISCHG(bool enable){ uint8_t regValue = this->read8(BQ25883_REG_INPUT_VOLTAGE_LIMIT); regValue = (regValue & 0b10111111); if(enable) regValue = (regValue | 0b01000000); this->write8(BQ25883_REG_INPUT_VOLTAGE_LIMIT, regValue); }
void BQ25883::setPFM_OOA_DIS(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_INPUT_VOLTAGE_LIMIT); regValue = (regValue & 0b11011111); if(disable) regValue = (regValue | 0b00100000); this->write8(BQ25883_REG_INPUT_VOLTAGE_LIMIT, regValue); }

void BQ25883::setInputCurrentLimit(float inputCurLimit){ uint8_t regValue = this->read8(BQ25883_REG_INPUT_CURRENT_LIMIT); regValue = (regValue & 0b11100000); if(inputCurLimit < 0.5) inputCurLimit = 0.5; else if(inputCurLimit > 3.3) inputCurLimit = 3.3; uint8_t inputCurrentLimit = lround(((inputCurLimit - 0.5)*1000.0)/100.0); inputCurrentLimit = (inputCurrentLimit & 0b00011111); regValue = (regValue | inputCurrentLimit); this->write8(BQ25883_REG_INPUT_CURRENT_LIMIT, regValue); }
void BQ25883::setFORCE_ICO(bool enable){ uint8_t regValue = this->read8(BQ25883_REG_INPUT_CURRENT_LIMIT); regValue = (regValue & 0b01111111); if(enable) regValue = (regValue | 0b10000000); this->write8(BQ25883_REG_INPUT_CURRENT_LIMIT, regValue); }
void BQ25883::setFORCE_INDET(bool enable){ uint8_t regValue = this->read8(BQ25883_REG_INPUT_CURRENT_LIMIT); regValue = (regValue & 0b10111111); if(enable) regValue = (regValue | 0b01000000); this->write8(BQ25883_REG_INPUT_CURRENT_LIMIT, regValue); }
void BQ25883::setEN_ICO(bool enable){ uint8_t regValue = this->read8(BQ25883_REG_INPUT_CURRENT_LIMIT); regValue = (regValue & 0b11011111); if(enable) regValue = (regValue | 0b00100000); this->write8(BQ25883_REG_INPUT_CURRENT_LIMIT, regValue); }

void BQ25883::setPrechargeCurrentLimit(float preCurrentLimit){ uint8_t regValue = this->read8(BQ25883_REG_PRE_TERM_CURRENT_LIMIT); regValue = (regValue & 0b00001111); if(preCurrentLimit < 0.05) preCurrentLimit = 0.05; else if(preCurrentLimit > 0.8) preCurrentLimit = 0.8; uint8_t prechargeCurrentLimit = lround(((preCurrentLimit - 0.05)*1000.0)/50.0); prechargeCurrentLimit = (prechargeCurrentLimit & 0b11110000); regValue = (regValue | prechargeCurrentLimit); this->write8(BQ25883_REG_PRE_TERM_CURRENT_LIMIT, regValue); }
void BQ25883::setTerminationCurrentLimit(float termCurrentLimit){ uint8_t regValue = this->read8(BQ25883_REG_PRE_TERM_CURRENT_LIMIT); regValue = (regValue & 0b11110000); if(termCurrentLimit < 0.05) termCurrentLimit = 0.05; else if(termCurrentLimit > 0.8) termCurrentLimit = 0.8; uint8_t terminationCurrentLimit = lround(((termCurrentLimit - 0.05)*1000.0)/50.0); terminationCurrentLimit = (terminationCurrentLimit & 0b00001111); regValue = (regValue | terminationCurrentLimit); this->write8(BQ25883_REG_PRE_TERM_CURRENT_LIMIT, regValue); }

void BQ25883::setEN_TERM(bool enable){ uint8_t regValue = this->read8(BQ25883_REG_CHARGER_CONTROL_1); regValue = (regValue & 0b01111111); if(enable) regValue = (regValue | 0b10000000); this->write8(BQ25883_REG_CHARGER_CONTROL_1, regValue); }
void BQ25883::setSTAT_DIS(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_CHARGER_CONTROL_1); regValue = (regValue & 0b10111111); if(disable) regValue = (regValue | 0b01000000); this->write8(BQ25883_REG_CHARGER_CONTROL_1, regValue); }
void BQ25883::setWATCHDOG(uint8_t timerSetting){ uint8_t regValue = this->read8(BQ25883_REG_CHARGER_CONTROL_1); regValue = (regValue & 0b11001111); switch(timerSetting){ case 0: break; case 2: regValue = (regValue | 0b00100000); break; case 3: regValue = (regValue | 0b00110000); break; default: regValue = (regValue | 0b00010000); break; } this->write8(BQ25883_REG_CHARGER_CONTROL_1, regValue); }
void BQ25883::setEN_TIMER(bool enable){ uint8_t regValue = this->read8(BQ25883_REG_CHARGER_CONTROL_1); regValue = (regValue & 0b11110111); if(enable) regValue = (regValue | 0b00001000); this->write8(BQ25883_REG_CHARGER_CONTROL_1, regValue); }
void BQ25883::setCHG_TIMER(uint8_t timerSetting){ uint8_t regValue = this->read8(BQ25883_REG_CHARGER_CONTROL_1); regValue = (regValue & 0b11111001); switch(timerSetting){ case 0: break; case 1: regValue = (regValue | 0b00000010); break; case 3: regValue = (regValue | 0b00000110); break; default: regValue = (regValue | 0b00000100); break; } this->write8(BQ25883_REG_CHARGER_CONTROL_1, regValue); }
void BQ25883::setTMR2X_EN(bool enable){ uint8_t regValue = this->read8(BQ25883_REG_CHARGER_CONTROL_1); regValue = (regValue & 0b11111110); if(enable) regValue = (regValue | 0b00000001); this->write8(BQ25883_REG_CHARGER_CONTROL_1, regValue); }
void BQ25883::setAUTO_INDET_EN(bool enable){ uint8_t regValue = this->read8(BQ25883_REG_CHARGER_CONTROL_2); regValue = (regValue & 0b10111111); if(enable) regValue = (regValue | 0b01000000); this->write8(BQ25883_REG_CHARGER_CONTROL_2, regValue); }
void BQ25883::setT_REG_THRESH(uint8_t thermalRegulationThreshold){ uint8_t regValue = this->read8(BQ25883_REG_CHARGER_CONTROL_2); regValue = (regValue & 0b11001111); switch(thermalRegulationThreshold){ case 0: break; case 1: regValue = (regValue | 0b00010000); break; case 2: regValue = (regValue | 0b00100000); break; default: regValue = (regValue | 0b00110000); break; } this->write8(BQ25883_REG_CHARGER_CONTROL_2, regValue); }
void BQ25883::setEN_CHG(bool enable){ uint8_t regValue = this->read8(BQ25883_REG_CHARGER_CONTROL_2); regValue = (regValue & 0b11110111); if(enable) regValue = (regValue | 0b00001000); this->write8(BQ25883_REG_CHARGER_CONTROL_2, regValue); }
void BQ25883::setCELLLOWV_THRESH(uint8_t cellLowThreshold){ uint8_t regValue = this->read8(BQ25883_REG_CHARGER_CONTROL_2); regValue = (regValue & 0b11111011); if(cellLowThreshold) regValue = (regValue | 0b00000100); this->write8(BQ25883_REG_CHARGER_CONTROL_2, regValue); }
void BQ25883::setVCELL_RECHG_THRESH_OFF(uint8_t rechargeThresholdOffset){ uint8_t regValue = this->read8(BQ25883_REG_CHARGER_CONTROL_2); regValue = (regValue & 0b11111100); switch(rechargeThresholdOffset){ case 0: break; case 2: regValue = (regValue | 0b00000010); break; case 3: regValue = (regValue | 0b00000011); break; default: regValue = (regValue | 0b00000001); break; } this->write8(BQ25883_REG_CHARGER_CONTROL_2, regValue); }
void BQ25883::setPFM_DIS(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_CHARGER_CONTROL_3); regValue = (regValue & 0b01111111); if(disable) regValue = (regValue | 0b10000000); this->write8(BQ25883_REG_CHARGER_CONTROL_3, regValue); }
void BQ25883::wdReset(){ uint8_t regValue = this->read8(BQ25883_REG_CHARGER_CONTROL_3); regValue = (regValue & 0b10111111); regValue = (regValue | 0b01000000); this->write8(BQ25883_REG_CHARGER_CONTROL_3, regValue); }
void BQ25883::setTOPOFF_TIMER(uint8_t topOffTimer){ uint8_t regValue = this->read8(BQ25883_REG_CHARGER_CONTROL_3); regValue = (regValue & 0b11001111); switch(topOffTimer){ case 1: regValue = (regValue | 0b00010000); break; case 2: regValue = (regValue | 0b00100000); break; case 3: regValue = (regValue | 0b00110000); break; default: break; } this->write8(BQ25883_REG_CHARGER_CONTROL_3, regValue); }
void BQ25883::setJEITA_VSET(uint8_t jeitaSetting){ uint8_t regValue = this->read8(BQ25883_REG_CHARGER_CONTROL_4); regValue = (regValue & 0b11100111); switch(jeitaSetting){ case 0: break; case 2: regValue = (regValue | 0b00010000); break; case 3: regValue = (regValue | 0b00011000); break; default: regValue = (regValue | 0b00001000); break; } this->write8(BQ25883_REG_CHARGER_CONTROL_4, regValue); }
void BQ25883::setJEITA_ISETH(uint8_t jeitaSetting){ uint8_t regValue = this->read8(BQ25883_REG_CHARGER_CONTROL_4); regValue = (regValue & 0b11111011); if(jeitaSetting) regValue = (regValue | 0b00000100); this->write8(BQ25883_REG_CHARGER_CONTROL_4, regValue); }
void BQ25883::setJEITA_ISETC(uint8_t jeitaSetting){ uint8_t regValue = this->read8(BQ25883_REG_CHARGER_CONTROL_4); regValue = (regValue & 0b11111100); switch(jeitaSetting){ case 0: break; case 2: regValue = (regValue | 0b00000010); break; case 3: regValue = (regValue | 0b00000011); break; default: regValue = (regValue | 0b00000001); break; } this->write8(BQ25883_REG_CHARGER_CONTROL_4, regValue); }

void BQ25883::setADC_DONE_MASK(bool disable){ uint16_t regValue = this->read16(BQ25883_REG_CHARGER_MASK_1); regValue = (regValue & 0b01111111); if(disable) regValue = (regValue | 0b10000000); this->write16(BQ25883_REG_CHARGER_MASK_1, regValue); }
void BQ25883::setIINDPM_MASK(bool disable){ uint16_t regValue = this->read16(BQ25883_REG_CHARGER_MASK_1); regValue = (regValue & 0b10111111); if(disable) regValue = (regValue | 0b01000000); this->write16(BQ25883_REG_CHARGER_MASK_1, regValue); }
void BQ25883::setVINDPM_MASK(bool disable){ uint16_t regValue = this->read16(BQ25883_REG_CHARGER_MASK_1); regValue = (regValue & 0b11011111); if(disable) regValue = (regValue | 0b00100000); this->write16(BQ25883_REG_CHARGER_MASK_1, regValue); }
void BQ25883::setT_REG_MASK(bool disable){ uint16_t regValue = this->read16(BQ25883_REG_CHARGER_MASK_1); regValue = (regValue & 0b11101111); if(disable) regValue = (regValue | 0b00010000); this->write16(BQ25883_REG_CHARGER_MASK_1, regValue); }
void BQ25883::setWD_MASK(bool disable){ uint16_t regValue = this->read16(BQ25883_REG_CHARGER_MASK_1); regValue = (regValue & 0b11110111); if(disable) regValue = (regValue | 0b00001000); this->write16(BQ25883_REG_CHARGER_MASK_1, regValue); }
void BQ25883::setCHRG_MASK(bool disable){ uint16_t regValue = this->read16(BQ25883_REG_CHARGER_MASK_1); regValue = (regValue & 0b11111000); if(disable) regValue = (regValue | 0b00000001); this->write16(BQ25883_REG_CHARGER_MASK_1, regValue); }
void BQ25883::setPG_MASK(bool disable){ uint16_t regValue = this->read16(BQ25883_REG_CHARGER_MASK_1); regValue = (regValue & 0b01111111); if(disable) regValue = (regValue | 0b10000000); this->write16(BQ25883_REG_CHARGER_MASK_1, regValue); }
void BQ25883::setVBUS_MASK(bool disable){ uint16_t regValue = this->read16(BQ25883_REG_CHARGER_MASK_1); regValue = (regValue & 0b10111111); if(disable) regValue = (regValue | 0b01000000); this->write16(BQ25883_REG_CHARGER_MASK_1, regValue); }
void BQ25883::setTS_MASK(bool disable){ uint16_t regValue = this->read16(BQ25883_REG_CHARGER_MASK_1); regValue = (regValue & 0b11011111); if(disable) regValue = (regValue | 0b00100000); this->write16(BQ25883_REG_CHARGER_MASK_1, regValue); }
void BQ25883::setICO_MASK(bool disable){ uint16_t regValue = this->read16(BQ25883_REG_CHARGER_MASK_1); regValue = (regValue & 0b11101111); if(disable) regValue = (regValue | 0b00010000); this->write16(BQ25883_REG_CHARGER_MASK_1, regValue); }

void BQ25883::setVBUS_OVP_MASK(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_FAULT_MASK); regValue = (regValue & 0b01111111); if(disable) regValue = (regValue | 0b10000000); this->write8(BQ25883_REG_FAULT_MASK, regValue); }
void BQ25883::setTSHUT_MASK(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_FAULT_MASK); regValue = (regValue & 0b10111111); if(disable) regValue = (regValue | 0b01000000); this->write8(BQ25883_REG_FAULT_MASK, regValue); }
void BQ25883::setTMR_MASK(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_FAULT_MASK); regValue = (regValue & 0b11101111); if(disable) regValue = (regValue | 0b00010000); this->write8(BQ25883_REG_FAULT_MASK, regValue); }
void BQ25883::setSNS_SHORT_MASK(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_FAULT_MASK); regValue = (regValue & 0b11110111); if(disable) regValue = (regValue | 0b00001000); this->write8(BQ25883_REG_FAULT_MASK, regValue); }

void BQ25883::setADC_EN(bool enable){ uint8_t regValue = this->read8(BQ25883_REG_ADC_CONTROL); regValue = (regValue & 0b01111111); if(enable) regValue = (regValue | 0b10000000); this->write8(BQ25883_REG_ADC_CONTROL, regValue); }
void BQ25883::setADC_ONE_SHOT(bool enable){ uint8_t regValue = this->read8(BQ25883_REG_ADC_CONTROL); regValue = (regValue & 0b10111111); if(enable) regValue = (regValue | 0b01000000); this->write8(BQ25883_REG_ADC_CONTROL, regValue); }
void BQ25883::setADC_SAMPLE_SPEED(uint8_t sampleSpeed){ uint8_t regValue = this->read8(BQ25883_REG_ADC_CONTROL); regValue = (regValue & 0b11001111); regValue = (regValue | ((sampleSpeed & 0x03) << 4)); this->write8(BQ25883_REG_ADC_CONTROL, regValue); }

void BQ25883::setIBUS_ADC_DIS(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_ADC_FUNCTION_DISABLE); regValue = (regValue & 0b01111111); if(disable) regValue = (regValue | 0b10000000); this->write8(BQ25883_REG_ADC_FUNCTION_DISABLE, regValue); }
void BQ25883::setICHG_ADC_DIS(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_ADC_FUNCTION_DISABLE); regValue = (regValue & 0b10111111); if(disable) regValue = (regValue | 0b01000000); this->write8(BQ25883_REG_ADC_FUNCTION_DISABLE, regValue); }
void BQ25883::setVBUS_ADC_DIS(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_ADC_FUNCTION_DISABLE); regValue = (regValue & 0b11011111); if(disable) regValue = (regValue | 0b00100000); this->write8(BQ25883_REG_ADC_FUNCTION_DISABLE, regValue); }
void BQ25883::setVBAT_ADC_DIS(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_ADC_FUNCTION_DISABLE); regValue = (regValue & 0b11101111); if(disable) regValue = (regValue | 0b00010000); this->write8(BQ25883_REG_ADC_FUNCTION_DISABLE, regValue); }
void BQ25883::setTS_ADC_DIS(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_ADC_FUNCTION_DISABLE); regValue = (regValue & 0b11111011); if(disable) regValue = (regValue | 0b00000100); this->write8(BQ25883_REG_ADC_FUNCTION_DISABLE, regValue); }
void BQ25883::setVCELL_ADC_DIS(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_ADC_FUNCTION_DISABLE); regValue = (regValue & 0b11111101); if(disable) regValue = (regValue | 0b00000010); this->write8(BQ25883_REG_ADC_FUNCTION_DISABLE, regValue); }
void BQ25883::setTDIE_ADC_DIS(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_ADC_FUNCTION_DISABLE); regValue = (regValue & 0b11111110); if(disable) regValue = (regValue | 0b00000001); this->write8(BQ25883_REG_ADC_FUNCTION_DISABLE, regValue); }

void BQ25883::registerReset(){ uint8_t regValue = this->read8(BQ25883_REG_PART_INFORMATION); regValue = (regValue & 0b01111111); regValue = (regValue | 0b10000000); this->write8(BQ25883_REG_PART_INFORMATION, regValue); }

// void BQ25883::setVDIFF_END_OFFSET(uint8_t cellBalExitThresh){ uint16_t regValue = this->read16(BQ25883_REG_CELL_BALANCING_CONTROL_1); regValue = (regValue & 0xFF1F); regValue |= ((cellBalExitThresh & 0x0F) << 5); this->write16(BQ25883_REG_CELL_BALANCING_CONTROL_1, regValue); }
// void BQ25883::setTCB_QUAL_INTERVAL(uint8_t cbQualInterval){ uint16_t regValue = this->read16(BQ25883_REG_CELL_BALANCING_CONTROL_1); if(cbQualInterval) regValue |= (1<<4); else regValue &= ~(1<<4); this->write16(BQ25883_REG_CELL_BALANCING_CONTROL_1, regValue); }
// void BQ25883::setTCB_ACTIVE(uint8_t tcbActive){ uint16_t regValue = this->read16(BQ25883_REG_CELL_BALANCING_CONTROL_1); regValue = (regValue & ~(0x000C)); regValue |= ((tcbActive & 0x03) << 2); this->write16(BQ25883_REG_CELL_BALANCING_CONTROL_1, regValue); }
// void BQ25883::setTSETTLE(uint8_t tSettle){ uint16_t regValue = this->read16(BQ25883_REG_CELL_BALANCING_CONTROL_1); regValue = (regValue & ~(0x0003)); regValue |= (tSettle & 0x03); this->write16(BQ25883_REG_CELL_BALANCING_CONTROL_1, regValue); }
// void BQ25883::setVQUAL_TH(uint8_t vQualThreshold){ uint16_t regValue = this->read16(BQ25883_REG_CELL_BALANCING_CONTROL_1); regValue = (regValue & ~(0xF000)); regValue |= ((vQualThreshold & 0x0F) << 12); this->write16(BQ25883_REG_CELL_BALANCING_CONTROL_1, regValue); }
// void BQ25883::setVDIFF_START(uint8_t vDiffStartThreshold){ uint16_t regValue = this->read16(BQ25883_REG_CELL_BALANCING_CONTROL_1); regValue = (regValue & ~(0x0F00)); regValue |= ((vDiffStartThreshold & 0x0F) << 8); this->write16(BQ25883_REG_CELL_BALANCING_CONTROL_1, regValue); }
// void BQ25883::setCB_CHG_DIS(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_CELL_BALANCING_STAT_CONT); if(disable) regValue |= (1<<7); else regValue &= ~(1<<7); this->write8(BQ25883_REG_CELL_BALANCING_STAT_CONT, regValue); }
// void BQ25883::setCB_AUTO_EN(bool enable){ uint8_t regValue = this->read8(BQ25883_REG_CELL_BALANCING_STAT_CONT); if(enable) regValue |= (1<<6); else regValue &= ~(1<<6); this->write8(BQ25883_REG_CELL_BALANCING_STAT_CONT, regValue); }

// void BQ25883::setQCBL_EN(bool enable){ uint8_t regValue = this->read8(BQ25883_REG_CELL_BALANCING_FLAG); if(enable) regValue |= (1<<6); else regValue &= ~(1<<6); this->write8(BQ25883_REG_CELL_BALANCING_FLAG, regValue); }
// void BQ25883::setQCBH_EN(bool enable){ uint8_t regValue = this->read8(BQ25883_REG_CELL_BALANCING_FLAG); if(enable) regValue |= (1<<7); else regValue &= ~(1<<7); this->write8(BQ25883_REG_CELL_BALANCING_FLAG, regValue); }

// void BQ25883::setCB_MASK(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_CELL_BALANCING_MASK); if(disable) regValue |= (1<<5); else regValue &= ~(1<<5); this->write8(BQ25883_REG_CELL_BALANCING_MASK, regValue); }
// void BQ25883::setHS_CV_MASK(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_CELL_BALANCING_MASK); if(disable) regValue |= (1<<4); else regValue &= ~(1<<4); this->write8(BQ25883_REG_CELL_BALANCING_MASK, regValue); }
// void BQ25883::setLS_CV_MASK(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_CELL_BALANCING_MASK); if(disable) regValue |= (1<<3); else regValue &= ~(1<<3); this->write8(BQ25883_REG_CELL_BALANCING_MASK, regValue); }
// void BQ25883::setHS_OV_MASK(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_CELL_BALANCING_MASK); if(disable) regValue |= (1<<2); else regValue &= ~(1<<2); this->write8(BQ25883_REG_CELL_BALANCING_MASK, regValue); }
// void BQ25883::setLS_OV_MASK(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_CELL_BALANCING_MASK); if(disable) regValue |= (1<<1); else regValue &= ~(1<<1); this->write8(BQ25883_REG_CELL_BALANCING_MASK, regValue); }
// void BQ25883::setCB_OC_MASK(bool disable){ uint8_t regValue = this->read8(BQ25883_REG_CELL_BALANCING_MASK); if(disable) regValue |= (1<<0); else regValue &= ~(1<<0); this->write8(BQ25883_REG_CELL_BALANCING_MASK, regValue); }



/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  _____ ___   _____
// |_   _|__ \ / ____|
//   | |    ) | |
//   | |   / /| |
//  _| |_ / /_| |____
// |_____|____|\_____|
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//private read/write methods
uint8_t BQ25883::read8(uint8_t reg){
    uint8_t byte0 = 0;
#ifndef HAL_I2C_MODULE_ENABLED
    Wire.beginTransmission(BQ25883_I2C_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false);

    Wire.requestFrom(BQ25883_I2C_ADDRESS, 1, true);
    byte0 = Wire.read();
#else
    uint8_t data[1];
    Board_I2C_Mem_Read(_handle, BQ25883_I2C_ADDRESS, reg, data, 1);
    byte0 = data[0];
#endif
    return byte0;
}

uint16_t BQ25883::read16(uint8_t reg){
    uint8_t byte0 = 0;
    uint8_t byte1 = 0;
#ifndef HAL_I2C_MODULE_ENABLED
    Wire.beginTransmission(BQ25883_I2C_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false);

    Wire.requestFrom(BQ25883_I2C_ADDRESS, 2, true);
    byte0 = Wire.read();
    byte1 = Wire.read();
#else
    uint8_t data[2];
    Board_I2C_Mem_Read(_handle, BQ25883_I2C_ADDRESS, reg, data, 2);
    byte0 = data[0];
    byte1 = data[1];
#endif
    return (byte1 << 8) | byte0;
}

uint32_t BQ25883::read32(uint8_t reg){
    uint8_t byte0 = 0;
    uint8_t byte1 = 0;
    uint8_t byte2 = 0;
    uint8_t byte3 = 0;
    uint32_t returnValue = 0;
#ifndef HAL_I2C_MODULE_ENABLED
    Wire.beginTransmission(BQ25883_I2C_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false);

    Wire.requestFrom(BQ25883_I2C_ADDRESS, 4, true);
    byte0 = Wire.read();
    byte1 = Wire.read();
    byte2 = Wire.read();
    byte3 = Wire.read();
#else
    uint8_t data[4];
    Board_I2C_Mem_Read(_handle, BQ25883_I2C_ADDRESS, reg, data, 4);
    byte0 = data[0];
    byte1 = data[1];
    byte2 = data[2];
    byte3 = data[3];
#endif
    returnValue += (uint32_t)byte3 << 24;
    returnValue += (uint32_t)byte2 << 16;
    returnValue += (uint32_t)byte1 << 8;
    returnValue += byte0;
    return returnValue;
}

int16_t BQ25883::readADC(uint8_t reg){
    uint8_t byte1 = 0;
    uint8_t byte0 = 0;
#ifndef HAL_I2C_MODULE_ENABLED
    Wire.beginTransmission(BQ25883_I2C_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false);

    Wire.requestFrom(BQ25883_I2C_ADDRESS, 2, true);
    byte1 = Wire.read();
    byte0 = Wire.read();
#else
    uint8_t data[2];
    Board_I2C_Mem_Read(_handle, BQ25883_I2C_ADDRESS, reg, data, 2);
    byte1 = data[0];
    byte0 = data[1];
#endif
    return (byte1 << 8) | byte0;
}

void BQ25883::write8(uint8_t reg, uint8_t data){
#ifndef HAL_I2C_MODULE_ENABLED
    Wire.beginTransmission(BQ25883_I2C_ADDRESS);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
#else
    Board_I2C_Mem_Write(_handle, BQ25883_I2C_ADDRESS, reg, &data, 1);
#endif
}

void BQ25883::write16(uint8_t reg, uint16_t data) {
#ifndef HAL_I2C_MODULE_ENABLED
    Wire.beginTransmission(BQ25883_I2C_ADDRESS);
    Wire.write(reg);
    Wire.write((uint8_t)(0xFF & (data >> 0))); // Byte0
    Wire.write((uint8_t)(0xFF & (data >> 8))); // Byte1
    Wire.endTransmission();
#else
    uint8_t dataArray[2];
    dataArray[0] = (uint8_t)(0xFF & (data >> 0)); // Byte0
    dataArray[1] = (uint8_t)(0xFF & (data >> 8)); // Byte1
    Board_I2C_Mem_Write(_handle, BQ25883_I2C_ADDRESS, reg, dataArray, 2);
#endif
}

void BQ25883::write32(uint8_t reg, uint32_t data) {
#ifndef HAL_I2C_MODULE_ENABLED
    Wire.beginTransmission(BQ25883_I2C_ADDRESS);
    Wire.write(reg);
    Wire.write((uint8_t)(0xFF & (data >> 0)));  // Byte0
    Wire.write((uint8_t)(0xFF & (data >> 8)));  // Byte1
    Wire.write((uint8_t)(0xFF & (data >> 16))); // Byte2
    Wire.write((uint8_t)(0xFF & (data >> 24))); // Byte3
    Wire.endTransmission();
#else
    uint8_t dataArray[4];
    dataArray[0] = (uint8_t)(0xFF & (data >> 0));  // Byte0
    dataArray[1] = (uint8_t)(0xFF & (data >> 8));  // Byte1
    dataArray[2] = (uint8_t)(0xFF & (data >> 16)); // Byte2
    dataArray[3] = (uint8_t)(0xFF & (data >> 24)); // Byte3
    Board_I2C_Mem_Write(_handle, BQ25883_I2C_ADDRESS, reg, dataArray, 4);
#endif
}