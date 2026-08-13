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
#include "ssd1306.h"
#include <string.h>

uint8_t STM32_SSD1306::gddram[SSD1306_HEIGHT/8][128];

#ifndef HAL_I2C_MODULE_ENABLED
#include "LL_i2c.h"
#endif

/* Write command */
#define SSD1306_WRITECOMMAND(command)      ssd1306_I2C_Write(SSD1306_I2C_ADDR, 0x00, (command))
/* Write data */
#define SSD1306_WRITEDATA(data)            ssd1306_I2C_Write(SSD1306_I2C_ADDR, 0x40, (data))

/* Absolute value */
#define ABS(x)   ((x) > 0 ? (x) : -(x))

#define pgm_read_byte(addr) (*(const unsigned char *)(addr)) ///< PROGMEM workaround for non-AVR
#define ssd1306_swap(a, b) (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))) ///< No-temp-var swap operation
#define vccstate 0x2

inline GFXglyph *pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c) {
#ifdef __AVR__
	return &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
#else
	// expression in __AVR__ section may generate "dereferencing type-punned
	// pointer will break strict-aliasing rules" warning In fact, on other
	// platforms (such as STM32) there is no need to do this pointer magic as
	// program memory may be read in a usual way So expression may be simplified
	return gfxFont->glyph + c;
#endif //__AVR__
}

#ifndef HAL_I2C_MODULE_ENABLED
STM32_SSD1306::STM32_SSD1306(I2C_TypeDef *handle, uint8_t devAddr)
{
	_handle = handle;
	_devAddr = devAddr;
}
#else
STM32_SSD1306::STM32_SSD1306()
{
}
#endif

/*
 * @brief  Initializes SSD1306 LCD
 * @param  None
 * @retval Initialization status:
 *           - 0: LCD was not detected on I2C port
 *           - > 0: LCD initialized OK and ready to use
 */
uint8_t STM32_SSD1306::SSD1306_begin(Board_I2C_Handle handle, uint8_t devAddr)
{
	_handle = handle;
	_devAddr = devAddr;

	// Init sequence
	SSD1306_WRITECOMMAND(SSD1306_DISPLAYOFF);                    // 0xAE
	SSD1306_WRITECOMMAND(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
	SSD1306_WRITECOMMAND(0x80);                                  // the suggested ratio 0x80

	SSD1306_WRITECOMMAND(SSD1306_SETMULTIPLEX);                  // 0xA8
	SSD1306_WRITECOMMAND(SSD1306_HEIGHT - 1);

	SSD1306_WRITECOMMAND(SSD1306_SETDISPLAYOFFSET);              // 0xD3
	SSD1306_WRITECOMMAND(0x0);                                   // no offset
	SSD1306_WRITECOMMAND(SSD1306_SETSTARTLINE | 0x0);            // line #0
	SSD1306_WRITECOMMAND(SSD1306_CHARGEPUMP);                    // 0x8D
	if (vccstate == SSD1306_EXTERNALVCC)
	{ SSD1306_WRITECOMMAND(0x10); }
	else
	{ SSD1306_WRITECOMMAND(0x14); }
	SSD1306_WRITECOMMAND(SSD1306_MEMORYMODE);                    // 0x20
	SSD1306_WRITECOMMAND(0x00);                                  // 0x0 act like ks0108
	SSD1306_WRITECOMMAND(SSD1306_SEGREMAP | 0x1);
	SSD1306_WRITECOMMAND(SSD1306_COMSCANDEC);

#if defined SSD1306_128_32
	SSD1306_WRITECOMMAND(SSD1306_SETCOMPINS);                    // 0xDA
	SSD1306_WRITECOMMAND(0x02);
	SSD1306_WRITECOMMAND(SSD1306_SETCONTRAST);                   // 0x81
	SSD1306_WRITECOMMAND(0x8F);

#elif defined SSD1306_128_64
	SSD1306_WRITECOMMAND(SSD1306_SETCOMPINS);                    // 0xDA
	SSD1306_WRITECOMMAND(0x12);
	SSD1306_WRITECOMMAND(SSD1306_SETCONTRAST);                   // 0x81
	if (vccstate == SSD1306_EXTERNALVCC)
	{ SSD1306_WRITECOMMAND(0x9F); }
	else
	{ SSD1306_WRITECOMMAND(0xCF); }

#elif defined SSD1306_96_16
	SSD1306_WRITECOMMAND(SSD1306_SETCOMPINS);                    // 0xDA
	SSD1306_WRITECOMMAND(0x2);   //ada x12
	SSD1306_WRITECOMMAND(SSD1306_SETCONTRAST);                   // 0x81
	if (vccstate == SSD1306_EXTERNALVCC)
	{ SSD1306_WRITECOMMAND(0x10); }
	else
	{ SSD1306_WRITECOMMAND(0xAF); }

#endif

	SSD1306_WRITECOMMAND(SSD1306_SETPRECHARGE);                  // 0xd9
	if (vccstate == SSD1306_EXTERNALVCC)
	{ SSD1306_WRITECOMMAND(0x22); }
	else
	{ SSD1306_WRITECOMMAND(0xF1); }
	SSD1306_WRITECOMMAND(SSD1306_SETVCOMDETECT);                 // 0xDB
	SSD1306_WRITECOMMAND(0x40);
	SSD1306_WRITECOMMAND(SSD1306_DISPLAYALLON_RESUME);           // 0xA4
	SSD1306_WRITECOMMAND(SSD1306_NORMALDISPLAY);                 // 0xA6

	SSD1306_WRITECOMMAND(SSD1306_DEACTIVATE_SCROLL);

	SSD1306_WRITECOMMAND(SSD1306_DISPLAYON);//--turn on oled panel

	/* Clear screen */
	SSD1306_Fill(SSD1306_COLOR_BLACK);

	/* Update screen */
	SSD1306_UpdateScreen();

	/* Set default values */
	CurrentX = 0;
	CurrentY = 0;
	Rotation = 0;
	gfxFont = NULL;
	_fontScale = 1;

	//SSD1306_SetRotation(SSD1306.Rotation);

	/* Return OK */
	return 1;
}

// DRAWING FUNCTIONS -------------------------------------------------------

/*
 * @brief  Draws pixel at desired location
 * @note   @ref SSD1306_UpdateScreen() must called after that in order to see updated LCD screen
 * @param  x: X location. This parameter can be a value between 0 and SSD1306_WIDTH - 1
 * @param  y: Y location. This parameter can be a value between 0 and SSD1306_HEIGHT - 1
 * @param  color: Color to be used for screen fill. This parameter can be a value of @ref SSD1306_COLOR_t enumeration
 * @retval None
 */
void STM32_SSD1306::SSD1306_DrawPixel(uint16_t x, uint16_t y, SSD1306_COLOR_t color)
{
	if((x >= 0) && (x < SSD1306_WIDTH) && (y >= 0) && (y < SSD1306_HEIGHT))
	{
		// Pixel is in-bounds. Rotate coordinates if needed.
		switch(Rotation) {
		case 1:
			ssd1306_swap(x, y);
			x = SSD1306_WIDTH - x - 1;
			break;
		case 2:
			x = SSD1306_WIDTH  - x - 1;
			y = SSD1306_HEIGHT - y - 1;
			break;
		case 3:
			ssd1306_swap(x, y);
			y = SSD1306_HEIGHT - y - 1;
			break;
		}
		/* Check if pixels are inverted */
		if (Inverted)
			color = (SSD1306_COLOR_t)!color;

#ifdef ADAFRUIT_BUFFER
		switch(color)
		{
		case SSD1306_COLOR_WHITE:   buffer[x + (y/8)*SSD1306_WIDTH] |=  (1 << (y&7)); break;
		case SSD1306_COLOR_BLACK:   buffer[x + (y/8)*SSD1306_WIDTH] &= ~(1 << (y&7)); break;
		case SSD1306_COLOR_INVERSE: buffer[x + (y/8)*SSD1306_WIDTH] ^=  (1 << (y&7)); break;
		}

#else
		uint8_t pixel = 0x01;
		uint8_t line = y>>3;
		uint8_t byte = pixel<<(y%8);
		if(color)
			gddram[line][x] |= byte;
		else
			gddram[line][x] &= ~byte;
#endif
	}
}


void STM32_SSD1306::SSD1306_Clear (void)
{
	SSD1306_Fill(SSD1306_COLOR_BLACK);
	//SSD1306_UpdateScreen();
}

/**
 * @brief  Draws line on LCD
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x0: Line X start point. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y0: Line Y start point. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  x1: Line X end point. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y1: Line Y end point. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  c: Color to be used. This parameter can be a value of @ref SSD1306_COLOR_t enumeration
 * @retval None
 */
void STM32_SSD1306::SSD1306_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, SSD1306_COLOR_t c)
{
	int16_t dx, dy, sx, sy, err, e2, i, tmp = 0;

	dx = (x0 < x1) ? (x1 - x0) : (x0 - x1);
	dy = (y0 < y1) ? (y1 - y0) : (y0 - y1);
	sx = (x0 < x1) ? 1 : -1;
	sy = (y0 < y1) ? 1 : -1;
	err = ((dx > dy) ? dx : -dy) / 2;

	if (dx == 0) {
		if (y1 < y0) {
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}

		if (x1 < x0) {
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}

		/* Vertical line */
		for (i = y0; i <= y1; i++) {
			SSD1306_DrawPixel(x0, i, c);
		}

		/* Return from function */
		return;
	}

	if (dy == 0) {
		if (y1 < y0) {
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}

		if (x1 < x0) {
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}

		/* Horizontal line */
		for (i = x0; i <= x1; i++) {
			SSD1306_DrawPixel(i, y0, c);
		}

		/* Return from function */
		return;
	}

	while (1) {
		SSD1306_DrawPixel(x0, y0, c);
		if (x0 == x1 && y0 == y1) {
			break;
		}
		e2 = err;
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		}
	}
}

/**
 * @brief  Draws rectangle on LCD
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x: Top left X start point. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y: Top left Y start point. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  w: Rectangle width in units of pixels
 * @param  h: Rectangle height in units of pixels
 * @param  c: Color to be used. This parameter can be a value of @ref SSD1306_COLOR_t enumeration
 * @retval None
 */
void STM32_SSD1306::SSD1306_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1306_COLOR_t c) {
	/* Check input parameters */
	if (
			x >= SSD1306_WIDTH ||
			y >= SSD1306_HEIGHT
	) {
		/* Return error */
		return;
	}

	/* Check width and height */
	if ((x + w) >= SSD1306_WIDTH) {
		w = SSD1306_WIDTH - x;
	}
	if ((y + h) >= SSD1306_HEIGHT) {
		h = SSD1306_HEIGHT - y;
	}

	/* Draw 4 lines */
	SSD1306_DrawLine(x, y, x + w, y, c);         /* Top line */
	SSD1306_DrawLine(x, y + h, x + w, y + h, c); /* Bottom line */
	SSD1306_DrawLine(x, y, x, y + h, c);         /* Left line */
	SSD1306_DrawLine(x + w, y, x + w, y + h, c); /* Right line */
}

/**
 * @brief  Draws filled rectangle on LCD
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x: Top left X start point. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y: Top left Y start point. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  w: Rectangle width in units of pixels
 * @param  h: Rectangle height in units of pixels
 * @param  c: Color to be used. This parameter can be a value of @ref SSD1306_COLOR_t enumeration
 * @retval None
 */
void STM32_SSD1306::SSD1306_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1306_COLOR_t c) {
	uint8_t i;

	/* Check input parameters */
	if (
			x >= SSD1306_WIDTH ||
			y >= SSD1306_HEIGHT
	) {
		/* Return error */
		return;
	}

	/* Check width and height */
	if ((x + w) >= SSD1306_WIDTH) {
		w = SSD1306_WIDTH - x;
	}
	if ((y + h) >= SSD1306_HEIGHT) {
		h = SSD1306_HEIGHT - y;
	}

	/* Draw lines */
	for (i = 0; i <= h; i++) {
		/* Draw lines */
		SSD1306_DrawLine(x, y + i, x + w, y + i, c);
	}
}

/*!
   @brief   Draw a rounded rectangle with no fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    r   Radius of corner rounding
    @param    color 16-bit 5-6-5 Color to draw with
 */
void STM32_SSD1306::SSD1306_drawRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, int8_t r, SSD1306_COLOR_t color)
{
	int8_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
	if (r > max_radius)
		r = max_radius;
	// smarter version
	SSD1306_DrawLine(x + r, y, w - (2 * r) + x + r, y, color);         			// Top
	SSD1306_DrawLine(x + r, y + h - 1, w - (2 * r) + x + r, y + h - 1, color); 	// Bottom
	SSD1306_DrawLine(x, y + r, x, h - (2 * r) + y + r, color);         			// Left
	SSD1306_DrawLine(x + w - 1, y + r, x + w - 1, h - (2 * r) + y + r, color); 	// Right
	// draw four corners
	SSD1306_drawCircleHelper(x + r, y + r, r, 1, color);
	SSD1306_drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
	SSD1306_drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
	SSD1306_drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

/*!
   @brief   Draw a rounded rectangle with fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    r   Radius of corner rounding
    @param    color 16-bit 5-6-5 Color to draw/fill with
 */
void STM32_SSD1306::SSD1306_fillRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, int8_t r, SSD1306_COLOR_t color)
{
	int8_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
	if (r > max_radius)
		r = max_radius;
	// smarter version
	SSD1306_DrawFilledRectangle(x + r, y, w - 2 * r, h, color);
	// draw four corners
	SSD1306_fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
	SSD1306_fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
}

/**
 * @brief  Draws triangle on LCD
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x1: First coordinate X location. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y1: First coordinate Y location. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  x2: Second coordinate X location. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y2: Second coordinate Y location. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  x3: Third coordinate X location. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y3: Third coordinate Y location. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  c: Color to be used. This parameter can be a value of @ref SSD1306_COLOR_t enumeration
 * @retval None
 */
void STM32_SSD1306::SSD1306_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
		uint16_t x3, uint16_t y3, SSD1306_COLOR_t color) {
	/* Draw lines */
	SSD1306_DrawLine(x1, y1, x2, y2, color);
	SSD1306_DrawLine(x2, y2, x3, y3, color);
	SSD1306_DrawLine(x3, y3, x1, y1, color);
}

void STM32_SSD1306::SSD1306_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
		uint16_t x3, uint16_t y3, SSD1306_COLOR_t color) {
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
			yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
			curpixel = 0;

	deltax = ABS(x2 - x1);
	deltay = ABS(y2 - y1);
	x = x1;
	y = y1;

	if (x2 >= x1) {
		xinc1 = 1;
		xinc2 = 1;
	} else {
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1) {
		yinc1 = 1;
		yinc2 = 1;
	} else {
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay){
		xinc1 = 0;
		yinc2 = 0;
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax;
	} else {
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	for (curpixel = 0; curpixel <= numpixels; curpixel++) {
		SSD1306_DrawLine(x, y, x3, y3, color);

		num += numadd;
		if (num >= den) {
			num -= den;
			x += xinc1;
			y += yinc1;
		}
		x += xinc2;
		y += yinc2;
	}
}

/**
 * @brief  Draws circle to STM buffer
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x: X location for center of circle. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y: Y location for center of circle. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  r: Circle radius in units of pixels
 * @param  c: Color to be used. This parameter can be a value of @ref SSD1306_COLOR_t enumeration
 * @retval None
 */
void STM32_SSD1306::SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r, SSD1306_COLOR_t c) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	SSD1306_DrawPixel(x0, y0 + r, c);
	SSD1306_DrawPixel(x0, y0 - r, c);
	SSD1306_DrawPixel(x0 + r, y0, c);
	SSD1306_DrawPixel(x0 - r, y0, c);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		SSD1306_DrawPixel(x0 + x, y0 + y, c);
		SSD1306_DrawPixel(x0 - x, y0 + y, c);
		SSD1306_DrawPixel(x0 + x, y0 - y, c);
		SSD1306_DrawPixel(x0 - x, y0 - y, c);

		SSD1306_DrawPixel(x0 + y, y0 + x, c);
		SSD1306_DrawPixel(x0 - y, y0 + x, c);
		SSD1306_DrawPixel(x0 + y, y0 - x, c);
		SSD1306_DrawPixel(x0 - y, y0 - x, c);
	}
}

/**
 * @brief  Draws filled circle to STM buffer
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x: X location for center of circle. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y: Y location for center of circle. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  r: Circle radius in units of pixels
 * @param  c: Color to be used. This parameter can be a value of @ref SSD1306_COLOR_t enumeration
 * @retval None
 */
void STM32_SSD1306::SSD1306_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, SSD1306_COLOR_t c) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	SSD1306_DrawPixel(x0, y0 + r, c);
	SSD1306_DrawPixel(x0, y0 - r, c);
	SSD1306_DrawPixel(x0 + r, y0, c);
	SSD1306_DrawPixel(x0 - r, y0, c);
	SSD1306_DrawLine(x0 - r, y0, x0 + r, y0, c);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		SSD1306_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, c);
		SSD1306_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, c);

		SSD1306_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, c);
		SSD1306_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, c);
	}
}

/*!
    @brief    Quarter-circle drawer, used to do circles and roundrects
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    cornername  Mask bit #1 or bit #2 to indicate which quarters of
   the circle we're doing
    @param    color 16-bit 5-6-5 Color to draw with
 */
void STM32_SSD1306::SSD1306_drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, SSD1306_COLOR_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		if (cornername & 0x4) {
			SSD1306_DrawPixel(x0 + x, y0 + y, color);
			SSD1306_DrawPixel(x0 + y, y0 + x, color);
		}
		if (cornername & 0x2) {
			SSD1306_DrawPixel(x0 + x, y0 - y, color);
			SSD1306_DrawPixel(x0 + y, y0 - x, color);
		}
		if (cornername & 0x8) {
			SSD1306_DrawPixel(x0 - y, y0 + x, color);
			SSD1306_DrawPixel(x0 - x, y0 + y, color);
		}
		if (cornername & 0x1) {
			SSD1306_DrawPixel(x0 - y, y0 - x, color);
			SSD1306_DrawPixel(x0 - x, y0 - y, color);
		}
	}
}

/*!
    @brief  Quarter-circle drawer with fill, used for circles and roundrects
    @param  x0       Center-point x coordinate
    @param  y0       Center-point y coordinate
    @param  r        Radius of circle
    @param  corners  Mask bits indicating which quarters we're doing
    @param  delta    Offset from center-point, used for round-rects
    @param  color    16-bit 5-6-5 Color to fill with
 */
void STM32_SSD1306::SSD1306_fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, SSD1306_COLOR_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;
	int16_t px = x;
	int16_t py = y;

	delta++; // Avoid some +1's in the loop

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		// These checks avoid double-drawing certain lines, important
		// for the SSD1306 library which has an INVERT drawing mode.
		if (x < (y + 1)) {
			if (corners & 1)
				SSD1306_DrawLine(x0 + x, y0 - y, x0 + x, (y0 - y) + (2 * y) + delta, color);
			if (corners & 2)
				SSD1306_DrawLine(x0 - x, y0 - y, x0 - x, (y0 - y) + (2 * y) + delta, color);
		}
		if (y != py) {
			if (corners & 1)
				SSD1306_DrawLine(x0 + py, y0 - px, x0 + py, (y0 - px) + (2 * px) + delta, color);
			if (corners & 2)
				SSD1306_DrawLine(x0 - py, y0 - px, x0 - py, (y0 - px) + (2 * px) + delta, color);
			py = y;
		}
		px = x;
	}
}

void STM32_SSD1306::SSD1306_InvertDisplay (int i)
{
	if (i) SSD1306_WRITECOMMAND (SSD1306_INVERTDISPLAY);

	else SSD1306_WRITECOMMAND (SSD1306_NORMALDISPLAY);

}

/**
 * @brief  Updates buffer from internal RAM to LCD
 * @note   This function must be called each time you do some changes to LCD, to update buffer from RAM to LCD
 * @param  None
 * @retval None
 */
void STM32_SSD1306::SSD1306_UpdateScreen(void)
{

	SSD1306_WRITECOMMAND(SSD1306_COLUMNADDR);
	SSD1306_WRITECOMMAND(0);   // Column start address (0 = reset)
	SSD1306_WRITECOMMAND(SSD1306_WIDTH-1); // Column end address (127 = reset)

	SSD1306_WRITECOMMAND(SSD1306_PAGEADDR);
	SSD1306_WRITECOMMAND(0); // Page start address (0 = reset)
	SSD1306_WRITECOMMAND(0xFF); // Page end address

	//SEND data in 16byte packs
	uint8_t buffer[17];
	buffer[0] = 0x40;
	for(uint16_t line=0; line<(SSD1306_HEIGHT/8); line++)
	{
		for(uint8_t x=0;x<128;x+=16)
		{
			for(uint8_t i=1;i<17;i++)
			{
				buffer[i] = gddram[line][x+i-1];
			}

#ifndef HAL_I2C_MODULE_ENABLED
			I2C_write(_handle, _devAddr, buffer, 17, 1000);
#else
			Board_I2C_Master_Transmit(_handle, _devAddr, buffer, 17);
#endif
		}
	}
}

/**
 * @brief  Toggles pixels invertion inside internal RAM
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  None
 * @retval None
 */
void STM32_SSD1306::SSD1306_ToggleInvert(void) {
#ifdef OG
	uint16_t i;

	/* Toggle invert */
	SSD1306.Inverted = !SSD1306.Inverted;

	/* Do memory toggle */
	for (i = 0; i < sizeof(SSD1306_Buffer); i++) {
		SSD1306_Buffer[i] = ~SSD1306_Buffer[i];
	}
#endif
}

/**
 * @brief  Fills entire LCD with desired color
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  Color: Color to be used for screen fill. This parameter can be a value of @ref SSD1306_COLOR_t enumeration
 * @retval None
 */
void STM32_SSD1306::SSD1306_Fill(SSD1306_COLOR_t color)
{
	/* Set memory */
	for(uint8_t i=0;i<(SSD1306_HEIGHT/8);i++)
		for(uint8_t j=0;j<128;j++)
			gddram[i][j]=(color == SSD1306_COLOR_BLACK) ? 0x00 : 0xFF;
}

void STM32_SSD1306::SSD1306_SetRotation(uint8_t x)
{
	uint8_t rotation = (x & 3);
	switch(rotation) {
	case 0:
		Rotation = 0;
		break;
	case 1:
		Rotation = 1;
		break;
	case 2:
		Rotation = 2;
		break;
	case 3:
		Rotation = 3;
		break;
	}
}


/**
 * @brief  Sets cursor pointer to desired location for strings
 * @param  x: X location. This parameter can be a value between 0 and SSD1306_WIDTH - 1
 * @param  y: Y location. This parameter can be a value between 0 and SSD1306_HEIGHT - 1
 * @retval None
 */
void STM32_SSD1306::SSD1306_GotoXY(uint16_t x, uint16_t y)
{
	/* Set write pointers */
	CurrentX = x;
	CurrentY = y;
}


/******************************/
/***** SCROLL FUNCTIONS *******/
/******************************/


void STM32_SSD1306::SSD1306_ScrollRight(uint8_t start_row, uint8_t end_row)
{
	SSD1306_WRITECOMMAND (SSD1306_RIGHT_HORIZONTAL_SCROLL);  // send 0x26
	SSD1306_WRITECOMMAND (0x00);  // send dummy
	SSD1306_WRITECOMMAND(start_row);  // start page address
	SSD1306_WRITECOMMAND(0X00);  // time interval 5 frames
	SSD1306_WRITECOMMAND(end_row);  // end page address
	SSD1306_WRITECOMMAND(0X00);
	SSD1306_WRITECOMMAND(0XFF);
	SSD1306_WRITECOMMAND (SSD1306_ACTIVATE_SCROLL); // start scroll
}


void STM32_SSD1306::SSD1306_ScrollLeft(uint8_t start_row, uint8_t end_row)
{
	SSD1306_WRITECOMMAND (SSD1306_LEFT_HORIZONTAL_SCROLL);  // send 0x26
	SSD1306_WRITECOMMAND (0x00);  // send dummy
	SSD1306_WRITECOMMAND(start_row);  // start page address
	SSD1306_WRITECOMMAND(0X00);  // time interval 5 frames
	SSD1306_WRITECOMMAND(end_row);  // end page address
	SSD1306_WRITECOMMAND(0X00);
	SSD1306_WRITECOMMAND(0XFF);
	SSD1306_WRITECOMMAND (SSD1306_ACTIVATE_SCROLL); // start scroll
}


void STM32_SSD1306::SSD1306_Scrolldiagright(uint8_t start_row, uint8_t end_row)
{
	SSD1306_WRITECOMMAND(SSD1306_SET_VERTICAL_SCROLL_AREA);  // sect the area
	SSD1306_WRITECOMMAND (0x00);   // write dummy
	SSD1306_WRITECOMMAND(SSD1306_HEIGHT);

	SSD1306_WRITECOMMAND(SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL);
	SSD1306_WRITECOMMAND (0x00);
	SSD1306_WRITECOMMAND(start_row);
	SSD1306_WRITECOMMAND(0X00);
	SSD1306_WRITECOMMAND(end_row);
	SSD1306_WRITECOMMAND (0x01);
	SSD1306_WRITECOMMAND (SSD1306_ACTIVATE_SCROLL);
}


void STM32_SSD1306::SSD1306_Scrolldiagleft(uint8_t start_row, uint8_t end_row)
{
	SSD1306_WRITECOMMAND(SSD1306_SET_VERTICAL_SCROLL_AREA);  // sect the area
	SSD1306_WRITECOMMAND (0x00);   // write dummy
	SSD1306_WRITECOMMAND(SSD1306_HEIGHT);

	SSD1306_WRITECOMMAND(SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL);
	SSD1306_WRITECOMMAND (0x00);
	SSD1306_WRITECOMMAND(start_row);
	SSD1306_WRITECOMMAND(0X00);
	SSD1306_WRITECOMMAND(end_row);
	SSD1306_WRITECOMMAND (0x01);
	SSD1306_WRITECOMMAND (SSD1306_ACTIVATE_SCROLL);
}


void STM32_SSD1306::SSD1306_Stopscroll(void)
{
	SSD1306_WRITECOMMAND(SSD1306_DEACTIVATE_SCROLL);
}


/******************************/
/******* GFX FUNCTIONS ********/
/******************************/


/**
 * @brief  Draws the Bitmap
 * @param  X:  X location to start the Drawing
 * @param  Y:  Y location to start the Drawing
 * @param  *bitmap : Pointer to the bitmap
 * @param  W : width of the image
 * @param  H : Height of the image
 * @param  color : 1-> white/blue, 0-> black
 */
void STM32_SSD1306::SSD1306_DrawBitmap(tImage* bitmap, int16_t xi, int16_t yi, uint8_t invert)
{
	uint8_t byte = 0;
	uint8_t tempX = xi;

	for (int y = 0; y < bitmap->Height; y++)
	{
		for (int x = 0; x < (bitmap->Width/bitmap->dataSize); x++)
		{
			if(!invert)
				byte = bitmap->data[(y * (bitmap->Width/bitmap->dataSize)) + x];
			else
				byte = ~(bitmap->data[(y * (bitmap->Width/bitmap->dataSize)) + x]);

			for (int z = 0; z < bitmap->dataSize; z++)
			{
				if ((byte << z) & 0x80)
				{
					SSD1306_DrawPixel(tempX + z , (CurrentY + y), SSD1306_COLOR_WHITE);
				}
				else
				{
					SSD1306_DrawPixel(tempX + z, (CurrentY + y), SSD1306_COLOR_BLACK);
				}
			}
			/* Increase pointer */
			tempX += bitmap->dataSize;
		}
		tempX = xi;
	}
}

void STM32_SSD1306::SSD1306_DrawBitmap(tImage* bitmap, uint8_t invert)
{
	uint8_t byte = 0;
	uint8_t tempX = CurrentX;

	for (int y = 0; y < bitmap->Height; y++)
	{
		for (int x = 0; x < (bitmap->Width/bitmap->dataSize); x++)
		{
			if(!invert)
				byte = bitmap->data[(y * (bitmap->Width/bitmap->dataSize)) + x];
			else
				byte = ~(bitmap->data[(y * (bitmap->Width/bitmap->dataSize)) + x]);

			for (int z = 0; z < bitmap->dataSize; z++)
			{
				if ((byte << z) & 0x80)
				{
					SSD1306_DrawPixel(tempX + z , (CurrentY + y), SSD1306_COLOR_WHITE);
				}
				else
				{
					SSD1306_DrawPixel(tempX + z, (CurrentY + y), SSD1306_COLOR_BLACK);
				}
			}
			/* Increase pointer */
			tempX += bitmap->dataSize;
		}
		tempX = CurrentX;
	}
}


/**
 * @brief  Puts character to internal RAM
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  ch: Character to be written
 * @param  *Font: Pointer to @ref FontDef_t structure with used font
 * @param  color: Color used for drawing. This parameter can be a value of @ref SSD1306_COLOR_t enumeration
 * @retval Character written
 */
char STM32_SSD1306::SSD1306_Putc(char ch, SSD1306_COLOR_t color)
{
	uint8_t xx, yy, bits = 0, bit = 0;
	GFXglyph *glyph = gfxFont->glyph + (ch - gfxFont->first);
	uint16_t bo = glyph->bitmapOffset;

	for(yy = 0; yy < glyph->height; yy++)
	{
		for(xx = 0; xx < glyph->width; xx++)
		{
			if (!(bit++ & 7))
			{
				bits = gfxFont->bitmap[bo++];
			}

			SSD1306_COLOR_t pixelColor = (bits & 0x80) ? color : (SSD1306_COLOR_t)!color;

			/* Every glyph metric (pixel position, offset) is defined for the
			 * unscaled font, so each source pixel is expanded into a
			 * _fontScale x _fontScale block at the scaled position. */
			int16_t baseX = CurrentX + (int16_t)(xx + glyph->xOffset) * _fontScale;
			int16_t baseY = CurrentY + (int16_t)(yy + glyph->yOffset + gfxFont->yAdvance) * _fontScale;

			for(uint8_t sy = 0; sy < _fontScale; sy++)
			{
				for(uint8_t sx = 0; sx < _fontScale; sx++)
				{
					SSD1306_DrawPixel(baseX + sx, baseY + sy, pixelColor);
				}
			}

			bits <<= 1;
		}
	}

	CurrentX += glyph->xAdvance * _fontScale;
	return ch;
}

/**
 * @brief  Puts string to internal RAM
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  *str: String to be written
 * @param  *Font: Pointer to @ref FontDef_t structure with used font
 * @param  color: Color used for drawing. This parameter can be a value of @ref SSD1306_COLOR_t enumeration
 * @retval Zero on success or character value when function failed
 */
char STM32_SSD1306::SSD1306_Puts(char* str, SSD1306_COLOR_t color, SSD1306_Overlay opaque)
{
	uint8_t char_count = 0;

	char_count = SSD1306_getStringWidth(str, strlen(str));

	if(opaque == SSD1306_TRANSPARENT_TEXT)
	{
		SSD1306_DrawFilledRectangle(CurrentX, CurrentY, CurrentX + char_count,
				CurrentY + gfxFont->yAdvance * _fontScale, SSD1306_COLOR_BLACK);
	}

	/* Write characters */
	while (*str)
	{
		/* Write character by character */
		if (SSD1306_Putc(*str, color) != *str) {
			/* Return error */
			return *str;
		}

		/* Increase string pointer */
		str++;
	}

	/* Everything OK, zero should be returned */
	return *str;
}

void STM32_SSD1306::SSD1306_setAlignment(uint8_t align)
{
	uint8_t side = (align & 2);
	switch(side) {
	case SSD1306_LEFT:
		Alignment = SSD1306_LEFT;
		break;
	case SSD1306_MIDDLE:
		Alignment = SSD1306_MIDDLE;
		break;
	case SSD1306_RIGHT:
		Alignment = SSD1306_RIGHT;
		break;
	}
}

void STM32_SSD1306::SSD1306_setFont(const GFXfont *f) {
	gfxFont = (GFXfont *)f;
}

/**
 * @brief  Scales all subsequently drawn text by this many multiples of the
 *         font's native glyph size (each source pixel becomes a scale x
 *         scale block). A scale of 1 renders the font at its native size.
 * @param  scale: multiplier applied to glyph pixels, advances and heights.
 *         0 is treated as 1 since a zero scale would draw nothing.
 */
void STM32_SSD1306::SSD1306_setFontSize(uint8_t scale)
{
	_fontScale = (scale == 0) ? 1 : scale;
}

uint8_t STM32_SSD1306::SSD1306_getStringWidth(char* str, uint16_t length)
{
	uint8_t count = 0;

	while(length--)
	{
		GFXglyph *glyph = gfxFont->glyph + (*str-32);

		count += glyph->xAdvance * _fontScale;
		str++;
	}
	return count;
}

uint8_t STM32_SSD1306::SSD1306_getStringHeight(char* str, uint16_t length)
{
	uint8_t lastHeight = 0;

	while(length--)
	{
		GFXglyph *glyph = gfxFont->glyph + (*str-32);

		if(lastHeight < glyph->height)
			lastHeight = glyph->height;

		str++;
	}
	return lastHeight * _fontScale;
}

uint8_t STM32_SSD1306::SSD1306_getCharPxLength(char ch)
{
	ch -= (uint8_t)gfxFont->first;
	GFXglyph *glyph = gfxFont->glyph + ch;

	return glyph->xAdvance * _fontScale;
}


// Dim the display
// dim = 1: display is dimmed
// dim = 0: display is normal
void STM32_SSD1306::SSD1306_Dim(uint8_t value)
{
	uint8_t contrast;

	if(value)
		contrast = 0; // Dimmed display
	else
	{
		if (vccstate == SSD1306_EXTERNALVCC)
			contrast = 0x9F;
		else
			contrast = 0xCF;
	}
	// the range of contrast to too small to be really useful
	// it is useful to dim the display
	SSD1306_WRITECOMMAND(SSD1306_SETCONTRAST);
	SSD1306_WRITECOMMAND(contrast);
}

void STM32_SSD1306::SSD1306_ON(void)
{
	SSD1306_WRITECOMMAND(0x8D);
	SSD1306_WRITECOMMAND(0x14);
	SSD1306_WRITECOMMAND(0xAF);
}

void STM32_SSD1306::SSD1306_OFF(void)
{
	SSD1306_WRITECOMMAND(0x8D);
	SSD1306_WRITECOMMAND(0x10);
	SSD1306_WRITECOMMAND(0xAE);
}

uint8_t STM32_SSD1306::SSD1306_GetXPos(void)
{
	return CurrentX;
}

uint8_t STM32_SSD1306::SSD1306_GetYPos(void)
{
	return CurrentY;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  _____ ___   _____
// |_   _|__ \ / ____|
//   | |    ) | |
//   | |   / /| |
//  _| |_ / /_| |____
// |_____|____|\_____|
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

void STM32_SSD1306::ssd1306_I2C_WriteMulti(uint8_t address, uint8_t reg, uint8_t* data, uint16_t count) {
	uint8_t dt[40];
	dt[0] = reg;
	uint8_t i;
	for(i = 0; i < count; i++)
		dt[i+1] = data[i];
#ifndef HAL_I2C_MODULE_ENABLED
	I2C_write(_handle, _devAddr, dt, count+1, 50);
#else
	Board_I2C_Master_Transmit(_handle, address, dt, count+1);
#endif
}


void STM32_SSD1306::ssd1306_I2C_Write(uint8_t address, uint8_t reg, uint8_t data) {
	uint8_t dt[2];
	dt[0] = reg;
	dt[1] = data;
#ifndef HAL_I2C_MODULE_ENABLED
	I2C_write(_handle, _devAddr, dt, 2, 10);
#else
	Board_I2C_Master_Transmit(_handle, address, dt, 2);
#endif
}
