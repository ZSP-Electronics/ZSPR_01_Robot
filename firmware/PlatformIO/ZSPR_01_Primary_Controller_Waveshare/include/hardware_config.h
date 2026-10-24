#pragma once

#define RGB_MAX_PIXEL_CLOCK_HZ  (30000000UL)

#define BOARD_TFT_WIDTH      (480)
#define BOARD_TFT_HEIGHT     (480)

#define BOARD_TFT_BL         (6)

#define BOARD_TFT_HSYNC      (38)
#define BOARD_TFT_VSYNC      (39)
#define BOARD_TFT_DE         (40)
#define BOARD_TFT_PCLK       (41)

// T-RGB physical connection is RGB666, but ESP only supports RGB565

// RGB data signal(
// DB0:BlUE LSB;DB5:BIUE MSB;
// DB6:GREEN LSB;DB11:GREEN,MSB;
// DB12:RED LSB;DB17:RED MSB)
#define BOARD_TFT_B0      (-1)   //B0
#define BOARD_TFT_B1      (5)   //B1
#define BOARD_TFT_B2      (45)   //B2
#define BOARD_TFT_B3      (48)   //B3
#define BOARD_TFT_B4      (47)   //B4
#define BOARD_TFT_B5      (21)   //B5    MSB

#define BOARD_TFT_G0      (14)   //G0
#define BOARD_TFT_G1      (13)   //G1
#define BOARD_TFT_G2      (12)   //G2
#define BOARD_TFT_G3      (11)   //G3
#define BOARD_TFT_G4     (10)   //G4
#define BOARD_TFT_G5     (9)    //G5    MSB

#define BOARD_TFT_R0     (-1)   //R0
#define BOARD_TFT_R1     (46)    //R1
#define BOARD_TFT_R2     (3)    //R2
#define BOARD_TFT_R3     (8)    //R3
#define BOARD_TFT_R4     (18)    //R4
#define BOARD_TFT_R5     (17)    //R5    MSB

// #define BOARD_TFT_RST        IO_PIN_10
// #define BOARD_TFT_CS         IO_PIN_11
#define BOARD_MOSI           (1)
#define BOARD_SCLK           (2)
#define BOARD_MISO           (-1)

#define BOARD_I2C_SDA        (15)
#define BOARD_I2C_SCL        (7)
