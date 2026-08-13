#ifndef __BOARD_H__
#define __BOARD_H__

/* Comment out a line to fall back to the HAL driver for that peripheral.
   Leave commented (default) to use the LL (Low Layer) driver. */
// #define GPIO_USE_HAL
// #define I2C_USE_HAL
// #define UART_USE_HAL

/* LL driver headers are not pulled in by stm32c0xx_hal.h, so include them
   here unconditionally -- harmless even for peripherals left on HAL. */
#include "stm32c0xx_ll_bus.h"
#include "stm32c0xx_ll_rcc.h"
#include "stm32c0xx_ll_gpio.h"
#include "stm32c0xx_ll_i2c.h"
#include "stm32c0xx_ll_usart.h"

#endif /* __BOARD_H__ */
