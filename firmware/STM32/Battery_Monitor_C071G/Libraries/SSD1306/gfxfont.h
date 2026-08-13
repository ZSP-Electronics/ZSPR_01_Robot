/*
 * custom_font.h
 *
 *  Created on: Dec 29, 2019
 *      Author: zacharypina
 */

#ifndef INC_CUSTOM_FONT_H_
#define INC_CUSTOM_FONT_H_

//#include "string.h"

/// Font data stored PER GLYPH
typedef struct {
	uint16_t bitmapOffset; ///< Pointer into GFXfont->bitmap
	uint8_t width;         ///< Bitmap dimensions in pixels
	uint8_t height;        ///< Bitmap dimensions in pixels
	uint8_t xAdvance;      ///< Distance to advance cursor (x axis)
	int8_t xOffset;        ///< X dist from cursor pos to UL corner
	int8_t yOffset;        ///< Y dist from cursor pos to UL corner
} GFXglyph;

/// Data stored for FONT AS A WHOLE
typedef struct {
	char name[10];
	uint8_t *bitmap;  ///< Glyph bitmaps, concatenated
	GFXglyph *glyph;  ///< Glyph array
	uint8_t first;    ///< ASCII extents (first char)
	uint8_t last;     ///< ASCII extents (last char)
	uint8_t yAdvance; ///< Newline distance (y axis)
} GFXfont;

//extern GFXfont Dialog_bold_12;

#endif /* INC_CUSTOM_FONT_H_ */
