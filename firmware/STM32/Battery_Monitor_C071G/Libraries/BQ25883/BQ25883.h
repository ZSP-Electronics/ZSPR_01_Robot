
#ifndef __BQ25883_H__
#define __BQ25883_H__

#include "stm32fxxx.h"
#include "i2c.h"

/* BQ25883 I2C Address */
#define BQ25883_I2C_ADDRESS 0x6B
/* PART INFO */
#define BQ25883_PARTINFO 0x19

/* REGISTER DEFINITIONS (from bq25883.pdf) */
#define BQ25883_REG_VOLTAGE_LIMIT             0x00    // (R/W)
#define BQ25883_REG_CHARGE_CURRENT_LIMIT      0x01    // (R/W)
#define BQ25883_REG_INPUT_VOLTAGE_LIMIT       0x02    // (R/W)
#define BQ25883_REG_INPUT_CURRENT_LIMIT       0x03    // (R/W)
#define BQ25883_REG_PRE_TERM_CURRENT_LIMIT    0x04    // (R/W)
#define BQ25883_REG_CHARGER_CONTROL_1         0x05    // (R/W)
#define BQ25883_REG_CHARGER_CONTROL_2         0x06    // (R/W)
#define BQ25883_REG_CHARGER_CONTROL_3         0x07    // (R/W)
#define BQ25883_REG_CHARGER_CONTROL_4         0x08    // (R/W)
#define BQ25883_REG_OTG_CONTROL               0x09    // (R/W)
#define BQ25883_REG_ICO_CURRENT_LIMIT         0x0A    // (R)
#define BQ25883_REG_CHARGER_STATUS_1          0x0B    // (R)
#define BQ25883_REG_CHARGER_STATUS_2          0x0C    // (R)
#define BQ25883_REG_NTC_STATUS                0x0D    // (R)
#define BQ25883_REG_FAULT_STATUS              0x0E    // (R)
#define BQ25883_REG_CHARGER_FLAG_1            0x0F    // (R)
#define BQ25883_REG_CHARGER_FLAG_2            0x10    // (R)
#define BQ25883_REG_FAULT_FLAG                0x11    // (R)
#define BQ25883_REG_CHARGER_MASK_1            0x12    // (R/W)
#define BQ25883_REG_CHARGER_MASK_2            0x13    // (R/W)
#define BQ25883_REG_FAULT_MASK                0x14    // (R/W)
#define BQ25883_REG_ADC_CONTROL               0x15    // (R/W)
#define BQ25883_REG_ADC_FUNCTION_DISABLE      0x16    // (R/W)
#define BQ25883_REG_IBUS_ADC_1                0x17    // (R)
#define BQ25883_REG_IBUS_ADC_0                0x18    // (R)
#define BQ25883_REG_ICHG_ADC_1                0x19    // (R)
#define BQ25883_REG_ICHG_ADC_0                0x1A    // (R)
#define BQ25883_REG_VBUS_ADC_1                0x1B    // (R)
#define BQ25883_REG_VBUS_ADC_0                0x1C    // (R)
#define BQ25883_REG_VBAT_ADC_1                0x1D    // (R)
#define BQ25883_REG_VBAT_ADC_0                0x1E    // (R)
#define BQ25883_REG_VSYS_ADC_1                0x1F    // (R)
#define BQ25883_REG_VSYS_ADC_0                0x20    // (R)
#define BQ25883_REG_TS_ADC_1                  0x21    // (R)
#define BQ25883_REG_TS_ADC_0                  0x22    // (R)
#define BQ25883_REG_TDIE_ADC_1                0x23    // (R)
#define BQ25883_REG_TDIE_ADC_0                0x24    // (R)
#define BQ25883_REG_PART_INFORMATION          0x25    // (R/W)
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.1 Battery Voltage Regulation Limit Register (Address =00h) [reset =A0h]
// Reset A0h
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.2 Charger Current Limit Register (Address =01h) [reset =5Eh]
// Reset 0h 1h 1Eh
// Note: ICHG >2.2A (2Ch) clamped to2.2A. ICHG <100mA (01h)
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.3 Input Voltage Limit Register (Address =02h) [reset =84h]
// Reset 1h 0h 0h 04h
// Note: VINDPM >5.5V (10h) clamped to5.5V. VINDPM register is
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.4 Input Current Limit Register (Address =03h) [reset =39h]
// Reset 0h 0h 1h 19h
// Note: IINDPM >3300 mA(1Ch) clamped to3300mA. Actual input
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.5 Precharge andTermination Current Limit Register (Address =04h) [reset =22h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.6 Charger Control 1Register (Address =05h) [reset =9Dh]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.7 Charger Control 2Register (Address =06h) [reset =7Dh]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.8 Charger Control 3Register (Address =07h) [reset =02h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.9 Charger Control 4Register (Address =08h) [reset =0Dh]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.10 OTG Control Register (Address =09h) [reset =F6h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.11 ICOCurrent Limit inUseRegister (Address =0Ah) [reset =XXh]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.12 Charger Status 1Register (Address =0Bh) [reset =XXh]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.13 Charger Status 2Register (Address =0Ch) [reset =XXh]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.14 NTC Status Register (Address =0Dh) [reset =0Xh]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.15 FAULT Status Register (Address =0Eh) [reset =XXh]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.16 Charger Flag 1Register (Address =0Fh) [reset =00h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.17 Charger Flag 2Register (Address =10h) [reset =00h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.18 FAULT Flag Register (Address =11h) [reset =00h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.19 Charger Mask 1Register (Address =12h) [reset =00h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.20 Charger Mask 2Register (Address =13h) [reset =00h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.21 FAULT Mask Register (Address =14h) [reset =00h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.22 ADC Control Register (Address =15h) [reset =30h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.23 ADC Function Disable Register (Address =16h) [reset =00h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.24 IBUS ADC 1Register (Address =17h) [reset =00h]
// 8.5.25 IBUS ADC 0Register (Address =18h) [reset =00h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.26 ICHG ADC 1Register (Address =19h) [reset =00h]
// 8.5.27 ICHG ADC 0Register (Address =1Ah) [reset =00h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.28 VBUS ADC 1Register (Address =1Bh) [reset =00h]
// 8.5.29 VBUS ADC 0Register (Address =1Ch) [reset =00h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.30 VBAT ADC 1Register (Address =1Dh) [reset =00h]
// 8.5.31 VBAT ADC 0Register (Address =1Eh) [reset =00h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.32 VSYS ADC 1Register (Address =1Fh) [reset =00h]
// 8.5.33 VSYS ADC 0Register (Address =20h) [reset =00h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.34 TSADC 1Register (Address =21h) [reset =00h]
// 8.5.35 TSADC 0Register (Address =22h) [reset =00h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.36 TDIE ADC 1Register (Address =23h) [reset =00h]
// 8.5.37 TDIE ADC 0Register (Address =24h) [reset =00h]
// Product Folder Links: BQ25883Submit Documentation Feedback Copyright ©2019, Texas Instruments Incorporated8.5.38 Part Information Register (Address =25h) [reset =18h]

class BQ25883 {
public:
    BQ25883();
#ifdef HAL_I2C_MODULE_ENABLED
    bool begin(Board_I2C_Handle handle);
#else
    bool begin();
#endif

    uint8_t readCellVoltageLimitReg();
    float getCellVoltageLimit();

    uint8_t readChargeCurrentLimitReg();
    float getChargeCurrentLimit();
    bool getHIZMode();
    bool getILIMPinFunction();

    uint8_t readInputVoltageLimitReg();
    float getInputVoltageLimit();
    bool getEN_VINDPM_RST();
    bool getEN_BAT_DISCHG();
    bool getPFM_OOA_DIS();

    uint8_t readInputCurrentLimitReg();
    float getInputCurrentLimit();
    bool getFORCE_ICO();
    bool getFORCE_INDET();
    bool getEN_ICO();

    uint8_t readPreTermCurrentLimitReg();
    float getPrechargeCurrentLimit();
    float getTerminationCurrentLimit();

    uint32_t readChargeControlSettingsReg();
    bool getEN_TERM();
    bool getSTAT_DIS();
    uint8_t getWATCHDOG();
    bool getEN_TIMER();
    uint8_t getCHG_TIMER();
    bool getTMR2X_EN();
    bool getAUTO_INDET_EN();
    uint8_t getT_REG_THRESH();
    bool getEN_CHG();
    float getCELLLOWV_THRESH();
    float getVCELL_RECHG_THRESH_OFF();
    bool getPFM_DIS();
    bool getWD_RST();
    uint8_t getTOPOFF_TIMER();
    float getJEITA_VSET();
    uint8_t getJEITA_ISETH();
    uint8_t getJEITA_ISETC();

    uint8_t readIcoCurrentLimitReg();
    float getICOCurrentLimit();

    uint16_t readChargeStatusReg();
    bool getIINDPM_STAT();
    bool getVINDPM_STAT();
    bool getTREG_STAT();
    bool getWD_STAT();
    uint8_t getCHRG_STAT();
    bool getPG_STAT();
    uint8_t getVBUS_STAT();
    uint8_t getICO_STAT();

    uint8_t readNtcStatusReg();
    uint8_t getNTCStatus();

    uint8_t readFaultStatusReg();
    bool getVBUS_OVP_STAT();
    bool getTSHUT_STAT();
    bool getTMR_STAT();

    uint16_t readChargeFlagsReg();
    bool getIINDPM_FLAG();
    bool getVINDPM_FLAG();
    bool getTREG_FLAG();
    bool getWD_FLAG();
    bool getCHRG_FLAG();
    bool getPG_FLAG();
    bool getVBUS_FLAG();
    bool getTS_FLAG();
    bool getICO_FLAG();

    uint8_t readFaultFlagsReg();
    bool getVBUS_OVP_FLAG();
    bool getTSHUT_FLAG();
    bool getTMR_FLAG();

    uint16_t readChargerIntMaskReg();
    bool getADC_DONE_MASK();
    bool getIINDPM_MASK();
    bool getVINDPM_MASK();
    bool getT_REG_MASK();
    bool getWD_MASK();
    bool getCHRG_MASK();
    bool getPG_MASK();
    bool getVBUS_MASK();
    bool getTS_MASK();
    bool getICO_MASK();

    uint8_t readFaultIntMaskReg();
    bool getVBUS_OVP_MASK();
    bool getTSHUT_MASK();
    bool getTMR_MASK();
    bool getSNS_SHORT_MASK();

    uint8_t readADCControlSettingsReg();
    bool getADC_EN();
    bool getADC_ONE_SHOT();
    uint8_t getADC_SAMPLE_SPEED();

    uint8_t readADCFuncDisSettingsReg();
    bool getIBUS_ADC_DIS();
    bool getICHG_ADC_DIS();
    bool getVBUS_ADC_DIS();
    bool getVBAT_ADC_DIS();
    bool getTS_ADC_DIS();
    bool getVCELL_ADC_DIS();
    bool getTDIE_ADC_DIS();

    int16_t readADCIbusReg();
    float getADC_IBUS();

    int16_t readADCIchgReg();
    float getADC_ICHG();

    int16_t readADCVbusReg();
    float getADC_VBUS();

    int16_t readADCVbatReg();
    float getADC_VBAT();

    int16_t readADCVSysReg();
    float getADC_VSYS();

    int16_t readADCVCellTopReg();
    float getADC_VCELLTOP();

    uint16_t readADCTsReg();
    float getADC_TS();

    uint16_t readADCTDieReg();
    float getADC_TDIE();

    int16_t readADCVCellBotReg();
    float getADC_VCELLBOT();

    uint8_t readPartInfoReg();
    bool getREG_RST();
    uint8_t getPartNumber();
    uint8_t getDevRev();

    // uint16_t readCellBalContSettingsReg();
    // float getVDIFF_END_OFFSET();
    // float getTCB_QUAL_INTERVAL();
    // float getTCB_ACTIVE();
    // float getTSETTLE();
    // float getVQUAL_TH();
    // float getVDIFF_START();

    // uint8_t readCellBalStatReg();
    // bool getCB_CHG_DIS();
    // bool getCB_AUTO_EN();
    // bool getCB_STAT();
    // bool getHS_CV_STAT();
    // bool getLS_CV_STAT();
    // bool getHS_OV_STAT();
    // bool getLS_OV_STAT();
    // bool getCB_OC_STAT();

    // uint8_t readCellBalFlagsReg();
    // bool getQCBH_EN();
    // bool getQCBL_EN();
    // bool getCB_FLAG();
    // bool getHS_CV_FLAG();
    // bool getLS_CV_FLAG();
    // bool getHS_OV_FLAG();
    // bool getLS_OV_FLAG();
    // bool getCB_OC_FLAG();

    // uint8_t readCellBalIntMaskReg();
    // bool getCB_MASK();
    // bool getHS_CV_MASK();
    // bool getLS_CV_MASK();
    // bool getHS_OV_MASK();
    // bool getLS_OV_MASK();
    // bool getCB_OC_MASK();

    void pollAllRegs();

    void setCellVoltageLimit(float voltLimit);
    void setChargeCurrentLimit(float chgCurrentLimit);
    void setHIZMode(bool enable);
    void setILIMPinFunction(bool enable);

    void setInputVoltageLimit(float inputVoltLimit);
    void setEN_VINDPM_RST(bool enable);
    void setEN_BAT_DISCHG(bool enable);
    void setPFM_OOA_DIS(bool disable);

    void setInputCurrentLimit(float inputCurLimit);
    void setFORCE_ICO(bool enable);
    void setFORCE_INDET(bool enable);
    void setEN_ICO(bool enable);

    void setPrechargeCurrentLimit(float preCurrentLimit);
    void setTerminationCurrentLimit(float termCurrentLimit);

    void setEN_TERM(bool enable);
    void setSTAT_DIS(bool disable);
    void setWATCHDOG(uint8_t timerSetting);
    void setEN_TIMER(bool enable);
    void setCHG_TIMER(uint8_t timerSetting);
    void setTMR2X_EN(bool enable);
    void setAUTO_INDET_EN(bool enable);
    void setT_REG_THRESH(uint8_t thermalRegulationThreshold);
    void setEN_CHG(bool enable);
    void setCELLLOWV_THRESH(uint8_t cellLowThreshold);
    void setVCELL_RECHG_THRESH_OFF(uint8_t rechargeThresholdOffset);
    void setPFM_DIS(bool disable);
    void wdReset();
    void setTOPOFF_TIMER(uint8_t topOffTimer);
    void setJEITA_VSET(uint8_t jeitaSetting);
    void setJEITA_ISETH(uint8_t jeitaSetting);
    void setJEITA_ISETC(uint8_t jeitaSetting);

    void setADC_DONE_MASK(bool disable);
    void setIINDPM_MASK(bool disable);
    void setVINDPM_MASK(bool disable);
    void setT_REG_MASK(bool disable);
    void setWD_MASK(bool disable);
    void setCHRG_MASK(bool disable);
    void setPG_MASK(bool disable);
    void setVBUS_MASK(bool disable);
    void setTS_MASK(bool disable);
    void setICO_MASK(bool disable);

    void setVBUS_OVP_MASK(bool disable);
    void setTSHUT_MASK(bool disable);
    void setTMR_MASK(bool disable);
    void setSNS_SHORT_MASK(bool disable);

    void setADC_EN(bool enable);
    void setADC_ONE_SHOT(bool enable);
    void setADC_SAMPLE_SPEED(uint8_t sampleSpeed);

    void setIBUS_ADC_DIS(bool disable);
    void setICHG_ADC_DIS(bool disable);
    void setVBUS_ADC_DIS(bool disable);
    void setVBAT_ADC_DIS(bool disable);
    void setTS_ADC_DIS(bool disable);
    void setVCELL_ADC_DIS(bool disable);
    void setTDIE_ADC_DIS(bool disable);

    void registerReset();

    void setVDIFF_END_OFFSET(uint8_t cellBalExitThresh);
    void setTCB_QUAL_INTERVAL(uint8_t cbQualInterval);
    void setTCB_ACTIVE(uint8_t tcbActive);
    void setTSETTLE(uint8_t tSettle);
    void setVQUAL_TH(uint8_t vQualThreshold);
    void setVDIFF_START(uint8_t vDiffStartThreshold);
    void setCB_CHG_DIS(bool disable);
    void setCB_AUTO_EN(bool enable);

    void setQCBL_EN(bool enable);
    void setQCBH_EN(bool enable);

    void setCB_MASK(bool disable);
    void setHS_CV_MASK(bool disable);
    void setLS_CV_MASK(bool disable);
    void setHS_OV_MASK(bool disable);
    void setLS_OV_MASK(bool disable);
    void setCB_OC_MASK(bool disable);

private:
#ifdef HAL_I2C_MODULE_ENABLED
    Board_I2C_Handle _handle;
#endif
    // raw register storage (subset matching BQ25887 style)
    uint8_t rawCellVoltageLimitReg;
    uint8_t rawChargeCurrentLimitReg;
    uint8_t rawInputVoltageLimitReg;
    uint8_t rawInputCurrentLimitReg;
    uint8_t rawPreTermCurrentLimitReg;
    uint32_t rawChargeControlSettingsReg;
    uint8_t rawIcoCurrentLimitReg;
    uint16_t rawChargeStatusReg;
    uint8_t rawNtcStatusReg;
    uint8_t rawFaultStatusReg;
    uint16_t rawChargeFlagsReg;
    uint8_t rawFaultFlagsReg;
    uint16_t rawChargerIntMaskReg;
    uint8_t rawFaultIntMaskReg;
    uint8_t rawADCControlSettingsReg;
    uint8_t rawADCFuncDisSettingsReg;
    int16_t rawADCIbusReg;
    int16_t rawADCIchgReg;
    int16_t rawADCVbusReg;
    int16_t rawADCVbatReg;
    int16_t rawADCVSysReg;
    int16_t rawADCVCellTopReg;
    uint16_t rawADCTsReg;
    uint16_t rawADCTDieReg;
    int16_t rawADCVCellBotReg;
    uint8_t rawPartInfoReg;
    // uint16_t rawCellBalContSettingsReg;
    // uint8_t rawCellBalStatReg;
    // uint8_t rawCellBalFlagsReg;
    // uint8_t rawCellBalIntMaskReg;

    //private read/write methods
    uint8_t read8(uint8_t reg);
    uint16_t read16(uint8_t reg);
    uint32_t read32(uint8_t reg);
    int16_t readADC(uint8_t reg);
    void write8(uint8_t reg, uint8_t data);
    void write16(uint8_t reg, uint16_t data);
    void write32(uint8_t reg, uint32_t data);
};

#endif
