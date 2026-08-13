/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.h
  * @brief   This file contains all the function prototypes for
  *          the i2c.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __I2C_H__
#define __I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdbool.h>
/* USER CODE END Includes */

extern I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN Private defines */
#ifdef I2C_USE_HAL
typedef I2C_HandleTypeDef *Board_I2C_Handle;
#define BOARD_I2C1 (&hi2c1)
#else
typedef I2C_TypeDef *Board_I2C_Handle;
#define BOARD_I2C1 I2C1
#endif
/* USER CODE END Private defines */

void MX_I2C1_Init(void);

/* USER CODE BEGIN Prototypes */
bool Board_I2C_Mem_Read(Board_I2C_Handle h, uint8_t devAddr, uint8_t memAddr, uint8_t *data, uint16_t len);
bool Board_I2C_Mem_Write(Board_I2C_Handle h, uint8_t devAddr, uint8_t memAddr, uint8_t *data, uint16_t len);
bool Board_I2C_Master_Transmit(Board_I2C_Handle h, uint8_t devAddr, uint8_t *data, uint16_t len);
bool Board_I2C_IsDeviceReady(Board_I2C_Handle h, uint8_t devAddr);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H__ */

