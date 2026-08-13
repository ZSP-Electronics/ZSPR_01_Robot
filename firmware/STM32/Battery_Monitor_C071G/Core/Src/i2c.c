/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   This file provides code for the configuration
  *          of the I2C instances.
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
/* Includes ------------------------------------------------------------------*/
#include "i2c.h"

/* USER CODE BEGIN 0 */
#define BOARD_I2C_TIMEOUT_MS 100U

#ifndef I2C_USE_HAL
static void MX_I2C1_Init_LL(void)
{
  LL_I2C_InitTypeDef I2C_InitStruct = {0};
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

  /* PB6 : I2C1_SCL, PB7 : I2C1_SDA (open-drain, AF6) */
  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_6;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
  I2C_InitStruct.Timing = 0x10805D88;
  I2C_InitStruct.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
  I2C_InitStruct.DigitalFilter = 0;
  I2C_InitStruct.OwnAddress1 = 0;
  I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
  I2C_InitStruct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;
  LL_I2C_Init(I2C1, &I2C_InitStruct);
  LL_I2C_Enable(I2C1);
}

/* Waits for TXIS (ready for next transmit byte) or NACK, up to BOARD_I2C_TIMEOUT_MS from 'start'. */
static bool ll_i2c_wait_txis(I2C_TypeDef *i2c, uint32_t start)
{
  while(!LL_I2C_IsActiveFlag_TXIS(i2c))
  {
    if(LL_I2C_IsActiveFlag_NACK(i2c)) return false;
    if((HAL_GetTick() - start) > BOARD_I2C_TIMEOUT_MS) return false;
  }
  return true;
}

static bool ll_i2c_wait_rxne(I2C_TypeDef *i2c, uint32_t start)
{
  while(!LL_I2C_IsActiveFlag_RXNE(i2c))
  {
    if(LL_I2C_IsActiveFlag_NACK(i2c)) return false;
    if((HAL_GetTick() - start) > BOARD_I2C_TIMEOUT_MS) return false;
  }
  return true;
}

static bool ll_i2c_wait_tc(I2C_TypeDef *i2c, uint32_t start)
{
  while(!LL_I2C_IsActiveFlag_TC(i2c))
  {
    if(LL_I2C_IsActiveFlag_NACK(i2c)) return false;
    if((HAL_GetTick() - start) > BOARD_I2C_TIMEOUT_MS) return false;
  }
  return true;
}

/* Best-effort: clears STOPF once seen, or gives up after the timeout so callers never hang forever. */
static void ll_i2c_wait_stop(I2C_TypeDef *i2c, uint32_t start)
{
  while(!LL_I2C_IsActiveFlag_STOP(i2c))
  {
    if((HAL_GetTick() - start) > BOARD_I2C_TIMEOUT_MS) break;
  }
  LL_I2C_ClearFlag_STOP(i2c);
}

bool Board_I2C_Mem_Write(Board_I2C_Handle h, uint8_t devAddr, uint8_t memAddr, uint8_t *data, uint16_t len)
{
  I2C_TypeDef *i2c = h;
  uint32_t start = HAL_GetTick();

  LL_I2C_HandleTransfer(i2c, devAddr << 1, LL_I2C_ADDRSLAVE_7BIT, 1 + len, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

  if(!ll_i2c_wait_txis(i2c, start)) { LL_I2C_ClearFlag_NACK(i2c); return false; }
  LL_I2C_TransmitData8(i2c, memAddr);

  for(uint16_t i = 0; i < len; i++)
  {
    if(!ll_i2c_wait_txis(i2c, start)) { LL_I2C_ClearFlag_NACK(i2c); return false; }
    LL_I2C_TransmitData8(i2c, data[i]);
  }

  ll_i2c_wait_stop(i2c, start);
  return true;
}

bool Board_I2C_Mem_Read(Board_I2C_Handle h, uint8_t devAddr, uint8_t memAddr, uint8_t *data, uint16_t len)
{
  I2C_TypeDef *i2c = h;
  uint32_t start = HAL_GetTick();

  /* Phase 1: write the register address, no STOP (SOFTEND) so a repeated START can follow */
  LL_I2C_HandleTransfer(i2c, devAddr << 1, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);
  if(!ll_i2c_wait_txis(i2c, start)) { LL_I2C_ClearFlag_NACK(i2c); return false; }
  LL_I2C_TransmitData8(i2c, memAddr);
  if(!ll_i2c_wait_tc(i2c, start)) { LL_I2C_ClearFlag_NACK(i2c); return false; }

  /* Phase 2: repeated START, read len bytes, AUTOEND generates the STOP */
  LL_I2C_HandleTransfer(i2c, devAddr << 1, LL_I2C_ADDRSLAVE_7BIT, len, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);
  for(uint16_t i = 0; i < len; i++)
  {
    if(!ll_i2c_wait_rxne(i2c, start)) { LL_I2C_ClearFlag_NACK(i2c); return false; }
    data[i] = LL_I2C_ReceiveData8(i2c);
  }

  ll_i2c_wait_stop(i2c, start);
  return true;
}

bool Board_I2C_Master_Transmit(Board_I2C_Handle h, uint8_t devAddr, uint8_t *data, uint16_t len)
{
  I2C_TypeDef *i2c = h;
  uint32_t start = HAL_GetTick();

  LL_I2C_HandleTransfer(i2c, devAddr << 1, LL_I2C_ADDRSLAVE_7BIT, len, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

  for(uint16_t i = 0; i < len; i++)
  {
    if(!ll_i2c_wait_txis(i2c, start)) { LL_I2C_ClearFlag_NACK(i2c); return false; }
    LL_I2C_TransmitData8(i2c, data[i]);
  }

  ll_i2c_wait_stop(i2c, start);
  return true;
}

bool Board_I2C_IsDeviceReady(Board_I2C_Handle h, uint8_t devAddr)
{
  I2C_TypeDef *i2c = h;
  uint32_t start = HAL_GetTick();

  LL_I2C_HandleTransfer(i2c, devAddr << 1, LL_I2C_ADDRSLAVE_7BIT, 0, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

  while(!LL_I2C_IsActiveFlag_STOP(i2c) && !LL_I2C_IsActiveFlag_NACK(i2c))
  {
    if((HAL_GetTick() - start) > BOARD_I2C_TIMEOUT_MS) return false;
  }

  bool nacked = LL_I2C_IsActiveFlag_NACK(i2c);
  if(nacked) LL_I2C_ClearFlag_NACK(i2c);
  if(LL_I2C_IsActiveFlag_STOP(i2c)) LL_I2C_ClearFlag_STOP(i2c);

  return !nacked;
}
#else
bool Board_I2C_Mem_Read(Board_I2C_Handle h, uint8_t devAddr, uint8_t memAddr, uint8_t *data, uint16_t len)
{
  return HAL_I2C_Mem_Read(h, devAddr << 1, memAddr, I2C_MEMADD_SIZE_8BIT, data, len, HAL_MAX_DELAY) == HAL_OK;
}

bool Board_I2C_Mem_Write(Board_I2C_Handle h, uint8_t devAddr, uint8_t memAddr, uint8_t *data, uint16_t len)
{
  return HAL_I2C_Mem_Write(h, devAddr << 1, memAddr, I2C_MEMADD_SIZE_8BIT, data, len, HAL_MAX_DELAY) == HAL_OK;
}

bool Board_I2C_Master_Transmit(Board_I2C_Handle h, uint8_t devAddr, uint8_t *data, uint16_t len)
{
  return HAL_I2C_Master_Transmit(h, devAddr << 1, data, len, 50) == HAL_OK;
}

bool Board_I2C_IsDeviceReady(Board_I2C_Handle h, uint8_t devAddr)
{
  return HAL_I2C_IsDeviceReady(h, devAddr << 1, 3, 1000) == HAL_OK;
}
#endif
/* USER CODE END 0 */

I2C_HandleTypeDef hi2c1;

/* I2C1 init function */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */
#ifndef I2C_USE_HAL
  MX_I2C1_Init_LL();
  return;
#endif
  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10805D88;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspInit 0 */

  /* USER CODE END I2C1_MspInit 0 */

  /** Initializes the peripherals clocks
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C1 clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
  /* USER CODE BEGIN I2C1_MspInit 1 */

  /* USER CODE END I2C1_MspInit 1 */
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspDeInit 0 */

  /* USER CODE END I2C1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();

    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);

  /* USER CODE BEGIN I2C1_MspDeInit 1 */

  /* USER CODE END I2C1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

