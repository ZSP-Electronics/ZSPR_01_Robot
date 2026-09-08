#pragma once

#define RGB_MAX_PIXEL_CLOCK_HZ  (8000000UL)

#define BOARD_TFT_WIDTH      (480)
#define BOARD_TFT_HEIGHT     (480)

#define BOARD_TFT_BL         IO_PIN_24

#define BOARD_TFT_HSYNC      (46)
#define BOARD_TFT_VSYNC      (15)
#define BOARD_TFT_DE         (16)
#define BOARD_TFT_PCLK       (17)

// T-RGB physical connection is RGB666, but ESP only supports RGB565

// RGB data signal(
// DB0:BlUE LSB;DB5:BIUE MSB;
// DB6:GREEN LSB;DB11:GREEN,MSB;
// DB12:RED LSB;DB17:RED MSB)
#define BOARD_TFT_DATA0      (-1)   //B0
#define BOARD_TFT_DATA1      (9)   //B1
#define BOARD_TFT_DATA2      (10)   //B2
#define BOARD_TFT_DATA3      (11)   //B3
#define BOARD_TFT_DATA4      (12)   //B4
#define BOARD_TFT_DATA5      (13)   //B5    MSB

#define BOARD_TFT_DATA6      (-1)   //G0
#define BOARD_TFT_DATA7      (21)   //G1
#define BOARD_TFT_DATA8      (47)   //G2
#define BOARD_TFT_DATA9      (48)   //G3
#define BOARD_TFT_DATA10     (45)   //G4
#define BOARD_TFT_DATA11     (0)    //G5    MSB

#define BOARD_TFT_DATA12     (-1)   //R0
#define BOARD_TFT_DATA13     (38)    //R1
#define BOARD_TFT_DATA14     (39)    //R2
#define BOARD_TFT_DATA15     (40)    //R3
#define BOARD_TFT_DATA16     (41)    //R4
#define BOARD_TFT_DATA17     (42)    //R5    MSB

#define BOARD_TFT_RST        IO_PIN_10
#define BOARD_TFT_CS         IO_PIN_11
#define BOARD_MOSI           (7)
#define BOARD_SCLK           (5)
#define BOARD_MISO           (6)

#define BOARD_I2C_SDA        (8)
#define BOARD_I2C_SCL        (18)

#define BOARD_TOUCH_IRQ      IO_PIN_12
#define BOARD_TOUCH_RST      IO_PIN_13

#define BOARD_SDMMC_CS       IO_PIN_14
#define BOARD_SDMMC_DET      IO_PIN_15
// #define BOARD_SDMMC_SCK      (39)
// #define BOARD_SDMMC_CMD      (40)
// #define BOARD_SDMMC_DAT      (38)

// #define BOARD_ADC_DET        (4)
#define BOARD_IO2_INT         (4)
#define BOARD_IO2_RST         (14)
#define BOARD_SERIAL_TX      (1)
#define BOARD_SERIAL_RX      (2)
#define BOARD_MOTOR_TX       (43)
#define BOARD_MOTOR_RX       (44)
#define BOARD_BUZZER         (3)

#define BOARD_IO1_INT        IO_PIN_0
#define BOARD_IO1_RST        IO_PIN_1
#define BOARD_VLx_SPI_N      IO_PIN_3
#define BOARD_VLx_NCS        IO_PIN_4
#define BOARD_VLx_SYNC       IO_PIN_5
#define BOARD_VLx_INT        IO_PIN_6
#define BOARD_VLx_LPN        IO_PIN_7
#define BOARD_SENSE_RST      IO_PIN_8
#define BOARD_SENSE_CHG      IO_PIN_9

#define BOARD_SYS_SPI_CS     IO_PIN_16
#define BOARD_SYS_PWR_BUT    IO_PIN_17
#define BOARD_SYS_PWR_EN     IO_PIN_18
#define BOARD_SYS_PWR_SHTDN  IO_PIN_20
#define BOARD_IMU_INT1       IO_PIN_21
#define BOARD_IMU_INT2       IO_PIN_22
#define BOARD_LED            IO_PIN_23
#define BOARD_CHG_EN         IO_PIN_25
#define BOARD_CHG_DETECT     IO_PIN_26
#define BOARD_CURR_ALERT1    IO_PIN_27
#define BOARD_CURR_ALERT2    IO_PIN_28
#define BOARD_REG_EN         IO_PIN_31
