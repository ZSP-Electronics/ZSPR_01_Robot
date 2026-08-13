/**
 * original author:  Tilen Majerle<tilen@majerle.eu>
 * modification for STM32f10x: Alexander Lutsai<s.lyra@ya.ru>

   ----------------------------------------------------------------------
   	Copyright (C) Alexander Lutsai, 2016
    Copyright (C) Tilen Majerle, 2015

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
   ----------------------------------------------------------------------
 */
#ifndef SSD1306_H
#define SSD1306_H 100

/**
 * This SSD1306 LCD uses I2C for communication
 *
 * Library features functions for drawing lines, rectangles and circles.
 *
 * It also allows you to draw texts and characters using appropriate functions provided in library.
 *
 * Default pinout
 *
 *
 *	COORDINATES for 128*32 pixels (x,y)
 *	 0,0|----------------------------|127,0
 *		|							 |
 *		|						  	 |
 *  0,31|----------------------------|127,31
 *
 *
SSD1306    |STM32F10x    |DESCRIPTION

VCC        |3.3V         |
GND        |GND          |
SCL        |             |Serial clock line
SDA        |             |Serial data line
 */

#include <stdio.h>
#include <string.h>

#include "stm32fxxx.h"
#include "i2c.h"
#include "gfxfont.h"
#include "bitmaps.h"

//#include "stdlib.h"
//#include "string.h"

//#define I2C_TEXT_EN
//#define ADAFRUIT_BUFFER

#define SSD1306_128_64
//#define SSD1306_128_32
//#define SSD1306_96_16

/* I2C address */
#ifndef SSD1306_I2C_ADDR
#define SSD1306_I2C_ADDR         0x3C
//#define SSD1306_I2C_ADDR       0x7A
#endif

#define NORMAL_Y_OFFSET	6
#define CUSTOM_Y_OFFSET	11

/* SSD1306 settings */
#if defined SSD1306_128_64
#define SSD1306_WIDTH                  128
#define SSD1306_HEIGHT                 64
#endif
#if defined SSD1306_128_32
#define SSD1306_WIDTH                  128
#define SSD1306_HEIGHT                 32
#endif
#if defined SSD1306_96_16
#define SSD1306_WIDTH                  96
#define SSD1306_HEIGHT                 16
#endif

#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF

#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA

#define SSD1306_SETVCOMDETECT 0xDB

#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9

#define SSD1306_SETMULTIPLEX 0xA8

#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10

#define SSD1306_SETSTARTLINE 0x40

#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22

#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8

#define SSD1306_SEGREMAP 0xA0

#define SSD1306_CHARGEPUMP 0x8D

#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2

// Scrolling #defines
#define SSD1306_ACTIVATE_SCROLL 0x2F
#define SSD1306_DEACTIVATE_SCROLL 0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL 0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL 0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A

#ifndef ssd1306_I2C_TIMEOUT
#define ssd1306_I2C_TIMEOUT					20000
#endif


/* Graphics  sorta*/
/* OLED Screen defines */
#define TOP_ROW       	0
#define BOTTOM_ROW    	SSD1306_HEIGHT - 16
#define LAST_ROW		SSD1306_HEIGHT - 1

#define FIRST_COLUMN    0
#define MIDDLE_COLUMN   SSD1306_WIDTH/2
#define LAST_COLUMN     SSD1306_WIDTH - 1

/**
 * @brief  SSD1306 color enumeration
 */
typedef enum {
	SSD1306_COLOR_BLACK = 0x00, /*!< Black color, no pixel */
	SSD1306_COLOR_WHITE = 0x01,  /*!< Pixel is set. Color depends on LCD */
	SSD1306_COLOR_INVERSE = 0x02
} SSD1306_COLOR_t;

typedef enum {
	SSD1306_LEFT = 0,
	SSD1306_MIDDLE = 1,
	SSD1306_RIGHT = 2
} SSD1306_Alignment;

typedef enum {
	SSD1306_OPAQUE = 0x00,
	SSD1306_TRANSPARENT_TEXT = 0x01
}SSD1306_Overlay;


class STM32_SSD1306
{
public:
#ifndef HAL_I2C_MODULE_ENABLED
	STM32_SSD1306(I2C_TypeDef *handle, uint8_t devAddr);
#else
	STM32_SSD1306();
#endif

	uint8_t SSD1306_begin(Board_I2C_Handle handle, uint8_t devAddr);
	void SSD1306_DrawPixel(uint16_t x, uint16_t y, SSD1306_COLOR_t color);
	void SSD1306_UpdateScreen(void);
	void SSD1306_ToggleInvert(void);
	void SSD1306_Fill(SSD1306_COLOR_t Color);
	void SSD1306_GotoXY(uint16_t x, uint16_t y);
	char SSD1306_Putc(char ch, SSD1306_COLOR_t color);
	char SSD1306_Puts(char* str, SSD1306_COLOR_t color, SSD1306_Overlay opaque);
	void SSD1306_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, SSD1306_COLOR_t c);
	void SSD1306_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1306_COLOR_t c);
	void SSD1306_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1306_COLOR_t c);
	void SSD1306_drawRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, int8_t r, SSD1306_COLOR_t color);
	void SSD1306_fillRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, int8_t r, SSD1306_COLOR_t color);
	void SSD1306_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, SSD1306_COLOR_t color);
	void SSD1306_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, SSD1306_COLOR_t color);
	void SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r, SSD1306_COLOR_t c);
	void SSD1306_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, SSD1306_COLOR_t c);
	void SSD1306_drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, SSD1306_COLOR_t color);
	void SSD1306_fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, SSD1306_COLOR_t color);
	void SSD1306_DrawBitmap(tImage* bitmap, int16_t xi, int16_t yi, uint8_t invert);
	void SSD1306_DrawBitmap(tImage* bitmap, uint8_t invert);
	void SSD1306_ScrollRight(uint8_t start_row, uint8_t end_row);
	void SSD1306_ScrollLeft(uint8_t start_row, uint8_t end_row);
	void SSD1306_Scrolldiagright(uint8_t start_row, uint8_t end_row);
	void SSD1306_Scrolldiagleft(uint8_t start_row, uint8_t end_row);
	void SSD1306_SetRotation(uint8_t x);
	void SSD1306_Stopscroll(void);
	void SSD1306_Dim(uint8_t value);
	void SSD1306_setFont(const GFXfont *f);
	void SSD1306_setFontSize(uint8_t scale);
	uint8_t SSD1306_getStringWidth(char* str, uint16_t length);
	uint8_t SSD1306_getStringHeight(char* str, uint16_t length);
	uint8_t SSD1306_getCharPxLength(char ch);
	void SSD1306_setAlignment(uint8_t align);
	void SSD1306_InvertDisplay (int i);
	void SSD1306_Clear (void);
	uint8_t SSD1306_GetXPos(void);
	uint8_t SSD1306_GetYPos(void);
	void SSD1306_ON(void);
	void SSD1306_OFF(void);

	uint8_t Inverted;

private:
#ifndef HAL_I2C_MODULE_ENABLED
	I2C_TypeDef *_handle;
#else
	Board_I2C_Handle _handle;
#endif
	uint8_t _devAddr;
	uint8_t CurrentX;
	uint8_t CurrentY;
	uint8_t Rotation;
	uint8_t Alignment;
	GFXfont	*gfxFont;
	uint8_t _fontScale = 1;
	static uint8_t gddram[SSD1306_HEIGHT/8][128];
	uint8_t *buffer;

	void ssd1306_I2C_Write(uint8_t address, uint8_t reg, uint8_t data);
	void ssd1306_I2C_WriteMulti(uint8_t address, uint8_t reg, uint8_t *data, uint16_t count);


};

#endif
