/*
 * LL_stmf1xx.h
 *
 *  Created on: May 11, 2020
 *      Author: zacharypina
 */

#ifndef DRIVERS_INC_STM32FXXX_H_
#define DRIVERS_INC_STM32FXXX_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "stm32c0xx_hal.h"

//#define FIXED_POINT_ARITHMETIC
//#define CDC_ENABLED

#define ENABLE_SERIAL_DEBUG
#define STM32FXXX

#ifdef __cplusplus
extern "C" {
#endif

#ifdef STM32FXXX
// #include "LL_delay.h"
#endif

typedef enum
{
	SYSTEM_ERROR 	= 0,
	SYSTEM_OK		= 1
}SYSTEM_STATUS;

#define TRUE 	1
#define FALSE	0
#define LOW GPIO_PIN_RESET
#define HIGH GPIO_PIN_SET

//#define min(a,b) ((a)<(b)?(a):(b))
//#define max(a,b) ((a)>(b)?(a):(b))
#define abs(x) ((x)>0?(x):-(x))
#define greaterThan(x,y) ((x)>(y)?1:0)
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))

#define pgm_read_word_near(addr) (*(const int *)(addr)) ///< PROGMEM workaround for non-AVR

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitToggle(value, bit) ((value) ^= (1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

#define map(x, in_min, in_max, out_min, out_max) ((x) - (in_min)) * ((out_max) - (out_min)) / ((in_max) - (in_min)) + (out_min)

#ifdef __cplusplus
}
#endif

//#include "stm32f1xx_hal.h"
#ifdef __cplusplus
#include "Serial.h"
#endif

#endif /* DRIVERS_INC_STM32FXXX_H_ */
