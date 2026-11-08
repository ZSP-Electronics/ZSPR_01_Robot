/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
#include "main.h"
#include "dma.h"
#include "gpio.h"
#include "i2c.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Battery_Link.h"
#include "Hardware_config.h"
#include "INA232.h"
#include "ssd1306.h"
#include "DialogBold10.h"
#include "stm32fxxx.h"
#include <BQ25883.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// I2C_HandleTypeDef hi2c1;
void update_button_states(void);
void readBatteryStatus(void);
bool estimateSystemLoadCurrent(float *currentAmps);
static void formatFloat2(float value, char *buf, size_t bufSize);
void serviceWatchdog(void);
void checkChargeFaults(void);
void checkBatteryStatPin(void);
bool configureCharger(void);
void softStartOutput(void);
void checkInputCurrentFaults(void);
void readStatFields(void);
void drawPrimaryScreen(void);
void drawPoweringOnScreen(void);
void drawPoweringDownScreen(void);
void outputInitOff(void);

#ifdef SERIAL_DEBUG
void printFirstFiveRegs(void);
void printAllFields(void);
void printBatteryStatus(void);
#endif
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
#define SYSTEM_OHM 0.005f // 5mOhm shunt resistor for system current
#define CURRENT_MAX 16.0f // maximum expected current in Amps for calibration

/* USER CODE BEGIN PD */
// HardwareSerial Serial(&huart1);
BQ25883 charger = BQ25883();
STM32_SSD1306 display;
Battery_Link batteryLink;
INA232 ina232 = INA232();

/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
bool battery_int_state = false;
bool left_button_state, middle_button_state, right_button_state = false;
bool previous_left_button_state, previous_middle_button_state, previous_right_button_state = false;
bool battery_pwr_good_state = false;
// Output power-on: soft start only runs after the middle button is held.
#define POWER_ON_HOLD_MS 2000UL
#define POWER_OFF_LOCKOUT_MS 5000UL // no power-off until this long after power-on
bool outputPowered = false;
uint32_t outputPowerOnTick = 0;
uint32_t middleHoldStartTick = 0;
bool middleHolding = false;
bool middleNeedRelease = false; // ignore the button until it is released
// uint8_t rxData[UART_BUFFER_SIZE];

// global variables for first five regsiters
float cellVoltageLimit = 0.0;

float chargeCurrentLimit = 0.0;
bool hizMode = true;
bool iLimPin = false;

float inputVoltageLimit = 0.0;
bool vInDpmRst = false;
bool batDischg = true;
bool pfmOoa = true;

float inputCurrentLimit = 0.0;
bool forceIco = true;
bool forceIndet = true;
bool enIco = false;

float prechargeCurrentLimit = 0.0;
float terminationCurrentLimit = 0.0;

float batteryVoltage = 0.0;
float batteryCurrent = 0.0;
bool ina232Ok = false;
float sysVoltage = 0.0; // V, from INA232
float sysCurrent = 0.0; // A, from INA232
float sysPower = 0.0;   // W, from INA232
float ext1Voltage = 0.0;
float ext2Voltage = 0.0;
float ext1Current = 0.0;
float ext2Current = 0.0;
uint32_t lastBatteryReadTick = 0;

bool chargeFaultActive = false;
// Set when the charge safety timer expires. Charging stays disabled until the
// input source is removed (PG drops), so the timer can't be defeated by
// toggling EN_CHG (which restarts it).
bool chargeTimerFaultLatched = false;
bool inputCurrentIssueActive = false;
bool displayExtPower = true;
/* USER CODE END PV */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#ifdef __cplusplus
extern "C" {
#endif

// int _write(int file, char *ptr, int len)
// {
//   HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
//   // HAL_UART_Transmit_IT(&huart1, (uint8_t*)ptr, len);
//   return len;
// }

// void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
// {
// 	if(GPIO_Pin == battery_int_pin)
// 	{
// 		battery_int_state = HAL_GPIO_ReadPin(battery_int);
// 	}
// }

#ifdef __cplusplus
}
#endif
/* USER CODE END PM */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// uint8_T TxData[50];
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  HAL_Delay(100);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  // /* USER CODE BEGIN 2 */
  Board_GPIO_WritePin(battery_cEnable, LOW);
  Serial.begin(BOARD_USART1);
#ifdef SERIAL_DEBUG
  Serial.println("Hello World!");
#endif

#ifdef ENABLE_OLED
  display.SSD1306_begin(BOARD_I2C1, SSD1306_I2C_ADDR);
  display.SSD1306_setFont(&Dialog_bold_10);
  display.SSD1306_setFontSize(1);
#endif

  if (charger.begin(BOARD_I2C1)) {
#ifdef SERIAL_DEBUG
    Serial.println(F("BQ25883 is working..."));
#endif
    // First I2C write moves the charger from default mode to host mode.
    bool cfgOk = configureCharger();
#ifdef SERIAL_DEBUG
    Serial.println(cfgOk ? F("BQ25883 configured, readback OK")
                         : F("!!! BQ25883 config readback MISMATCH !!!"));
#else
    (void)cfgOk;
#endif
    charger.readCellVoltageLimitReg(); // cache VREG so getCellVoltageLimit()
                                       // reflects the actual charger setting,
                                       // not a zeroed default
  } else {
// If BQ25883 is not working, blink the LED to indicate an error
#ifdef SERIAL_DEBUG
    Serial.println(F("BQ25883 is NOT working.  Check your wires..."));
#endif
    while (1) {
      Board_GPIO_TogglePin(user_led);
      HAL_Delay(500);
    }
  }

  // INA232 on the pack/system shunt: 5mOhm * 16A = 80mV, inside the default
  // +/-81.92mV ADC range, so no ADCRANGE change is needed.
  ina232Ok = ina232.begin(BOARD_I2C1) && ina232.setCalibration(SYSTEM_OHM, CURRENT_MAX);
#ifdef SERIAL_DEBUG
  Serial.println(ina232Ok ? F("INA232 is working...")
                          : F("INA232 is NOT working.  Check your wires..."));
#endif

#ifdef ENABLE_OLED
  drawPrimaryScreen();
#endif

  // Everything is configured. The output stays off until the middle button is
  // held for POWER_ON_HOLD_MS, then it is brought up gradually (soft start).
  outputInitOff();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {

    // Service the Primary Controller link: answer REQUEST_VOLTAGE frames and
    // latch any SEND_CURRENT_POWER rail data pushed from the Primary.
    batteryLink.poll();

    if (batteryLink.has_current_power()) {
      const Battery_Link::RailReadings &r = batteryLink.rails();
      ext1Voltage = r.sysBus_mV / 1000.0f;
      ext1Current = r.sysCurrent_mA / 1000.0f;
      ext2Voltage = r.motorBus_mV / 1000.0f;
      ext2Current = r.motorCurrent_mA / 1000.0f;
    }

    update_button_states();

    // Hold the middle button to power the output on, or (once it has been on
    // for at least POWER_OFF_LOCKOUT_MS) to power it off.
    {
      const uint32_t now = HAL_GetTick();
      if (Board_GPIO_ReadPin(button_middle)) { // released
        middleHolding = false;
        middleNeedRelease = false;
      } else if (!middleNeedRelease) { // pressed
        if (!middleHolding) {
          middleHolding = true;
          middleHoldStartTick = now;
        } else if (now - middleHoldStartTick >= POWER_ON_HOLD_MS) {
          if (!outputPowered) {
#ifdef ENABLE_OLED
            drawPoweringOnScreen();
#endif
            Board_GPIO_WritePin(user_led, HIGH);
            softStartOutput();
            outputPowered = true;
            outputPowerOnTick = HAL_GetTick();
            middleHolding = false;
            middleNeedRelease = true; // this hold must not count as power-off
            lastBatteryReadTick = 0;  // refresh readings right away
#ifdef ENABLE_OLED
            drawPrimaryScreen();
#endif
          } else if (now - outputPowerOnTick >= POWER_OFF_LOCKOUT_MS) {
#ifdef ENABLE_OLED
            drawPoweringDownScreen();
#endif
            Board_GPIO_WritePin(pwr_activation, LOW);
            Board_GPIO_WritePin(user_led, LOW);
#ifdef ENABLE_OLED
            HAL_Delay(1000); // leave the message visible briefly
#endif
            outputPowered = false;
            middleHolding = false;
            middleNeedRelease = true;
            lastBatteryReadTick = 0;
#ifdef ENABLE_OLED
            drawPrimaryScreen();
#endif
          }
          // else: within the lockout, keep waiting while held
        }
      }
    }

    if (HAL_GetTick() - lastBatteryReadTick >= 1000) {
      lastBatteryReadTick = HAL_GetTick();
      if (!outputPowered)
        HAL_GPIO_TogglePin(user_led); // standby heartbeat; solid on once powered
      serviceWatchdog();
      checkChargeFaults();
      checkInputCurrentFaults();
      readBatteryStatus();
// printBatteryStatus();
#ifdef ENABLE_OLED
      drawPrimaryScreen();
#endif

    }

    // Sampled every pass (not on the 1 s tick) so the 1 Hz fault blink is seen.
    checkBatteryStatPin();
  }
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType =
      RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

void update_button_states(void) {
  bool left_but_temp = !Board_GPIO_ReadPin(button_left);
  bool middle_but_temp = !Board_GPIO_ReadPin(button_middle);
  bool right_but_temp = !Board_GPIO_ReadPin(button_right);
  // battery_pwr_good_state = Board_GPIO_ReadPin(pwr_button_read);

  // Left Debounce
  if (left_but_temp) {
    if (!previous_left_button_state) {
      left_button_state = true;
      previous_left_button_state = left_button_state;
      Serial.println("Left Button Pressed");
    } else
      left_button_state = false;
  } else if (previous_left_button_state && !left_button_state) {
    previous_left_button_state = left_button_state;
  } else if (previous_left_button_state && left_button_state)
    left_button_state = false;

  // Middle Debounce
  if (middle_but_temp) {
    if (!previous_middle_button_state) {
      middle_button_state = true;
      previous_middle_button_state = middle_button_state;
      Serial.println("Middle Button Pressed");
    } else
      middle_button_state = false;
  } else if (previous_middle_button_state && !middle_button_state) {
    previous_middle_button_state = middle_button_state;
  } else if (previous_middle_button_state && middle_button_state)
    middle_button_state = false;

  // Right Debounce
  if (right_but_temp) {
    if (!previous_right_button_state) {
      right_button_state = true;
      previous_right_button_state = right_button_state;
      Serial.println("Right Button Pressed");
    } else
      right_button_state = false;
  } else if (previous_right_button_state && !right_button_state) {
    previous_right_button_state = right_button_state;
  } else if (previous_right_button_state && right_button_state)
    right_button_state = false;
}

void readBatteryStatus() {
  if (ina232Ok) {
    sysVoltage = ina232.readBusVoltage_mV() / 1000.0f;
    sysCurrent = ina232.readCurrent_mA() / 1000.0f;
    sysPower = ina232.readPower_mW() / 1000.0f;
    batteryVoltage = sysVoltage;
  } else {
    // INA232 missing - fall back to the BQ25883 ADC so the display still works.
    charger.readADCVbatReg();
    batteryVoltage = charger.getADC_VBAT();
  }

  // Cache the pack voltage (mV) the link hands back on REQUEST_VOLTAGE.
  batteryLink.set_battery_voltage_mV(
      (uint16_t)(batteryVoltage * 1000.0f + 0.5f));

  // charger.readADCIbusReg();
  // batteryCurrent = charger.getADC_IBUS();
}

void printBatteryStatus() {
  Serial.print(F("Charge Voltage: "));
  Serial.print(charger.readADCVbusReg());
  Serial.println(F(" mV"));
  Serial.print(F("Charge Current: "));
  Serial.print(charger.readADCIbusReg());
  Serial.println(F(" mA"));
}

// Formats to 2 decimal places using integer math only - this MCU's nano.specs
// libc has no float printf support, so "%f" would silently print nothing.
static void formatFloat2(float value, char *buf, size_t bufSize) {
  bool negative = value < 0.0f;
  if (negative)
    value = -value;
  value += 0.005f;

  unsigned long wholePart = (unsigned long)value;
  unsigned int fracPart = (unsigned int)((value - (float)wholePart) * 100.0f);

  snprintf(buf, bufSize, "%s%lu.%02u", negative ? "-" : "", wholePart,
           fracPart);
}

void serviceWatchdog() {
  // WD_RST must be re-written before the 40s watchdog window (REG05) expires,
  // otherwise the IC returns to default mode and resets VREG, ICHG, the timer
  // settings, AUTO_INDET_EN, JEITA and ADC_EN to their power-on values.
  // Read WD_STAT *before* kicking, so an expiry is seen on this pass.
  charger.readChargeStatusReg();
  if (charger.getWD_STAT()) {
#ifdef SERIAL_DEBUG
    Serial.println(F("!!! WATCHDOG EXPIRED - charger settings reverted to "
                     "defaults, re-applying config !!!"));
#endif
    configureCharger(); // the write itself returns the chip to host mode
  } else {
    charger.wdReset();
  }
}

// The BQ25883 has no TS-ignore bit (checked datasheet SLUSDL3A, REG08/0D/etc.),
// so the closest firmware option is to make the JEITA "warm" (45-60C) and
// "cool" (0-10C) windows behave like normal charging. Charging still suspends
// when TS reads Cold (<0C) or Hot (>60C), including a floating/open TS pin.
// This board has a real NTC (TH1 10k, divider R5 5.23k / R6 30.1k per the
// datasheet-recommended design), so the chip's default JEITA protection is
// correct and this override is OFF. Set to 1 only to ignore warm/cool limits.
#define ALLOW_CHARGE_ON_NTC_WARM_COOL 0
// ---------------------------------------------------------------------------
// Output soft start.
// The output switch is Q1 (SUD50P08-25L, P-channel) between SYS_BATT and
// SYS_OUT, pulled on by Q2 (2N7002K) from SYS_OUT_EN = PA15 (pwr_activation).
// High = output on. Slamming it on charges the robot's bulk capacitance with
// a large inrush spike, so instead PWM the enable pin with a duty cycle that
// ramps 0 -> 100% over SOFTSTART_RAMP_MS, then hold it high.
//
// Software PWM is used so no timer / CubeMX changes are needed; the call
// blocks for SOFTSTART_RAMP_MS (the Primary link and charger watchdog are not
// serviced meanwhile - fine for <1 s, the watchdog window is 40 s).
// ---------------------------------------------------------------------------
#define SOFTSTART_RAMP_MS   400UL  // total ramp time
#define SOFTSTART_PERIOD_US 200UL  // PWM period (5 kHz)

static uint32_t micros32(void) {
  uint32_t ms, ticks;
  do {
    ms = HAL_GetTick();
    ticks = SysTick->LOAD - SysTick->VAL; // counts up within the current ms
  } while (ms != HAL_GetTick());
  return ms * 1000UL + ticks / (SystemCoreClock / 1000000UL);
}

static void delayUntilUs(uint32_t target) {
  while ((int32_t)(micros32() - target) < 0) {
  }
}

// Drive SYS_OUT_EN low as a push-pull output (output off).
void outputInitOff(void) {
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = GPIO_PIN_15;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  Board_GPIO_WritePin(pwr_activation, LOW);
  HAL_GPIO_Init(GPIOA, &gpio);
}

void softStartOutput(void) {
  outputInitOff();

  const uint32_t rampUs = SOFTSTART_RAMP_MS * 1000UL;
  const uint32_t startUs = micros32();
  uint32_t elapsedUs;

  while ((elapsedUs = micros32() - startUs) < rampUs) {
    const uint32_t onUs = (SOFTSTART_PERIOD_US * elapsedUs) / rampUs;
    const uint32_t periodStart = micros32();
    if (onUs > 0) {
      Board_GPIO_WritePin(pwr_activation, HIGH);
      delayUntilUs(periodStart + onUs);
    }
    Board_GPIO_WritePin(pwr_activation, LOW);
    delayUntilUs(periodStart + SOFTSTART_PERIOD_US);
  }

  Board_GPIO_WritePin(pwr_activation, HIGH); // fully on
#ifdef SERIAL_DEBUG
  Serial.println(F("Output soft start complete"));
#endif
}

// ---------------------------------------------------------------------------
// BQ25883 host-mode configuration (datasheet SLUSDL3A, section 8.5).
// 2S pack: VREG / SYS_MIN / BATLOWV are pack voltages, not per-cell.
// Written as whole registers at startup and after any watchdog expiry.
// ---------------------------------------------------------------------------
// REG00: VREG = 6.8 V + 10 mV/LSB -> 0xA0 = 8.40 V (4.20 V/cell)
#define BQ_REG00_VREG       0xA0
// REG01: EN_HIZ=0, EN_ILIM=1 (R8 500 ohm, ~2.2 A backstop), ICHG=10*50mA=0.5 A
#define BQ_REG01_ICHG       0x4A
// REG02: EN_VINDPM_RST=1, EN_BAT_DISCHG=0, PFM_OOA_DIS=0, VINDPM=3.9+0.4=4.3 V
#define BQ_REG02_VINDPM     0x84
// REG03: FORCE_ICO=0, FORCE_INDET=0, EN_ICO=0, IINDPM=0.5 A (board input limit)
#define BQ_REG03_IINDPM     0x00
// REG04: IPRECHG=150 mA, ITERM=150 mA (50 mA + 50 mA/LSB each)
#define BQ_REG04_PRE_TERM   0x22
// REG05: EN_TERM=1, STAT_DIS=0, WATCHDOG=40 s, EN_TIMER=1, CHG_TIMER=12 h,
//        TMR2X_EN=1
#define BQ_REG05_CTRL1      0x9D
// REG06: EN_OTG=0, AUTO_INDET_EN=0 (D+/D- detection would overwrite IINDPM on
//        every plug-in), TREG=120 C, EN_CHG (bit 3, set at runtime),
//        BATLOWV=6.0 V, VRECHG=200 mV below VREG
#define BQ_REG06_CTRL2      0x35
#define BQ_REG06_EN_CHG     0x08
// REG07: PFM enabled, WD_RST=0 (self-clearing), TOPOFF off, SYS_MIN=6.2 V
#define BQ_REG07_CTRL3      0x02
// REG08: BHOT/BCOLD defaults (OTG only); JEITA warm/cool settings
#if ALLOW_CHARGE_ON_NTC_WARM_COOL
#define BQ_REG08_CTRL4      0x1F  // warm: VREG unchanged, 100% ICHG; cool: 100% ICHG
#else
#define BQ_REG08_CTRL4      0x0D  // warm: VREG 8.0 V, 100% ICHG; cool: 20% ICHG
#endif
// REG12: mask ADC_DONE, IINDPM, VINDPM (always input-limited at 0.5 A, would
//        spam INT); keep TREG, WD, CHRG interrupts
#define BQ_REG12_MASK1      0xE0
// REG13: mask ICO (disabled); keep PG, VBUS, TS, VSYS interrupts
#define BQ_REG13_MASK2      0x02
// REG14: all fault interrupts enabled (VBUS_OVP, TSHUT, BATOVP, TMR, SYS_SHORT)
#define BQ_REG14_FMASK      0x00
// REG15: ADC_EN=1, continuous conversion, ADC_SAMPLE=11 (default speed)
#define BQ_REG15_ADC        0xB0
// REG16: all ADC channels enabled (bit 1 is reserved and reads 1)
#define BQ_REG16_ADC_DIS    0x02

bool configureCharger() {
  const uint8_t reg06 =
      BQ_REG06_CTRL2 | (chargeTimerFaultLatched ? 0 : BQ_REG06_EN_CHG);

  struct RegCfg {
    uint8_t reg;
    uint8_t value;
    uint8_t verifyMask; // bits compared on readback
  };
  const RegCfg cfg[] = {
      {BQ25883_REG_VOLTAGE_LIMIT, BQ_REG00_VREG, 0xFF},
      {BQ25883_REG_CHARGE_CURRENT_LIMIT, BQ_REG01_ICHG, 0xFF},
      {BQ25883_REG_INPUT_VOLTAGE_LIMIT, BQ_REG02_VINDPM, 0xFF},
      {BQ25883_REG_INPUT_CURRENT_LIMIT, BQ_REG03_IINDPM, 0x3F},
      {BQ25883_REG_PRE_TERM_CURRENT_LIMIT, BQ_REG04_PRE_TERM, 0xFF},
      {BQ25883_REG_CHARGER_CONTROL_1, BQ_REG05_CTRL1, 0xFF},
      {BQ25883_REG_CHARGER_CONTROL_2, reg06, 0xFF},
      {BQ25883_REG_CHARGER_CONTROL_3, BQ_REG07_CTRL3, 0xBF},
      {BQ25883_REG_CHARGER_CONTROL_4, BQ_REG08_CTRL4, 0xFF},
      {BQ25883_REG_CHARGER_MASK_1, BQ_REG12_MASK1, 0xF9},
      {BQ25883_REG_CHARGER_MASK_2, BQ_REG13_MASK2, 0x97},
      {BQ25883_REG_FAULT_MASK, BQ_REG14_FMASK, 0xF9},
      {BQ25883_REG_ADC_CONTROL, BQ_REG15_ADC, 0xF0},
      {BQ25883_REG_ADC_FUNCTION_DISABLE, BQ_REG16_ADC_DIS, 0xFD},
  };

  for (const RegCfg &c : cfg)
    charger.writeRegister(c.reg, c.value);

  bool ok = true;
  for (const RegCfg &c : cfg) {
    const uint8_t got = charger.readRegister(c.reg);
    if ((got ^ c.value) & c.verifyMask) {
      ok = false;
#ifdef SERIAL_DEBUG
      char line[48];
      snprintf(line, sizeof(line), "  BQ reg 0x%02X wrote 0x%02X read 0x%02X",
               c.reg, c.value, got);
      Serial.println(line);
#endif
    }
  }
  return ok;
}

void checkChargeFaults() {
  charger.readFaultStatusReg();
  charger.readChargeStatusReg();

  const bool vbusOvp = charger.getVBUS_OVP_STAT();
  const bool tshut = charger.getTSHUT_STAT();
  const bool batOvp = charger.getBATOVP_STAT();
  const bool tmrFault = charger.getTMR_STAT();
  const bool powerGood = charger.getPG_STAT();

  // Safety timer: latch charging off. Toggling EN_CHG restarts the timer, so
  // only re-enable once the input source has been removed (new charge session).
  if (tmrFault && !chargeTimerFaultLatched) {
    chargeTimerFaultLatched = true;
    charger.setEN_CHG(false);
#ifdef SERIAL_DEBUG
    Serial.println(F("!!! SAFETY TIMER EXPIRED - charging latched off until "
                     "input is unplugged !!!"));
#endif
  } else if (chargeTimerFaultLatched && !powerGood) {
    chargeTimerFaultLatched = false;
    charger.setEN_CHG(true);
#ifdef SERIAL_DEBUG
    Serial.println(F("Input removed - safety timer latch cleared"));
#endif
  }

  // VBUS_OVP, TSHUT and BATOVP are handled by the charger itself (it stops
  // switching / charging and resumes when the condition clears). Report them
  // only - toggling EN_CHG here would needlessly restart the safety timer.
  const bool newFault = vbusOvp || tshut || batOvp || chargeTimerFaultLatched;

  if (newFault && !chargeFaultActive) {
    chargeFaultActive = true;
#ifdef SERIAL_DEBUG
    Serial.println(F("!!! CHARGE FAULT DETECTED !!!"));
    if (vbusOvp)
      Serial.println(F("  - VBUS Overvoltage"));
    if (tshut)
      Serial.println(F("  - Thermal Shutdown"));
    if (batOvp)
      Serial.println(F("  - Battery Overvoltage"));
    if (chargeTimerFaultLatched)
      Serial.println(F("  - Safety Timer Expired"));
#endif
  } else if (!newFault && chargeFaultActive) {
    chargeFaultActive = false;
#ifdef SERIAL_DEBUG
    Serial.println(F("Charge fault cleared"));
#endif
  }
}

// Decodes the BQ25883 STAT pin (open-drain):
//   low            -> charging
//   1 Hz blinking  -> fault (fault registers are read and printed)
//   high           -> not charging (done / disabled / no source)
// Call every main-loop pass; the blink is detected from edge spacing.
void checkBatteryStatPin() {
  enum StatState { STAT_UNKNOWN, STAT_CHARGING, STAT_IDLE, STAT_FAULT };
  static StatState state = STAT_UNKNOWN;
  static bool lastLevel = false;
  static bool levelInit = false;
  static uint32_t lastEdgeTick = 0;
  static uint8_t quickEdges = 0;     // consecutive edges < 800 ms apart
  static uint32_t lastFaultReport = 0;

  const uint32_t now = HAL_GetTick();
  const bool level = HAL_GPIO_ReadPin(battery_stat_pin) == GPIO_PIN_SET;

  // PG is open-drain, active low (R17 pull-up to +3.3V). With no good input
  // REGN is off, TS reads 0 V and the charger flags a TS fault and blinks
  // STAT even though nothing can charge. Treat that as plain "no input" and
  // restart blink detection once input returns.
  const bool inputGood = HAL_GPIO_ReadPin(power_good) == GPIO_PIN_RESET;
  if (!inputGood) {
    levelInit = false;
    quickEdges = 0;
    if (state != STAT_IDLE)
      Serial.println(F("Battery STAT: Not charging (no input power)"));
    state = STAT_IDLE;
    return;
  }

  if (!levelInit) {
    levelInit = true;
    lastLevel = level;
    lastEdgeTick = now;
  } else if (level != lastLevel) {
    lastLevel = level;
    quickEdges = (now - lastEdgeTick < 800) ? (uint8_t)(quickEdges + 1) : 1;
    lastEdgeTick = now;
  }

  StatState next;
  if (quickEdges >= 2 && now - lastEdgeTick < 1000) {
    next = STAT_FAULT;                       // still toggling -> blinking
  } else if (now - lastEdgeTick >= 1000) {
    quickEdges = 0;                          // steady level
    next = level ? STAT_IDLE : STAT_CHARGING;
  } else {
    return;                                  // single recent edge: undecided yet
  }

  const bool entering = (next != state);
  state = next;

  switch (state) {
  case STAT_CHARGING:
    if (entering)
      Serial.println(F("Battery STAT: Charging"));
    break;
  case STAT_IDLE:
    if (entering)
      Serial.println(F("Battery STAT: Not charging (done, disabled or no source)"));
    break;
  case STAT_FAULT:
    if (entering || now - lastFaultReport >= 5000) {
      lastFaultReport = now;
      charger.readFaultStatusReg();
      charger.readNtcStatusReg();
      charger.readChargeStatusReg();
      Serial.println(F("!!! Battery STAT: FAULT (pin blinking at 1 Hz) !!!"));
      Serial.println(charger.getVBUS_OVP_STAT() ? F("  - VBUS Overvoltage")
                                                : F("  - VBUS OVP: none"));
      Serial.println(charger.getTSHUT_STAT() ? F("  - Thermal Shutdown")
                                             : F("  - Thermal shutdown: none"));
      Serial.println(charger.getTMR_STAT() ? F("  - Safety Timer Expired")
                                           : F("  - Safety timer: ok"));
      Serial.println(charger.getBATOVP_STAT() ? F("  - Battery Overvoltage")
                                              : F("  - Battery OVP: none"));
      Serial.print(F("  - NTC status: "));
      switch (charger.getNTCStatus()) {
      case 0:  Serial.println(F("Normal")); break;
      case 2:  Serial.println(F("Warm"));   break;
      case 3:  Serial.println(F("Cool"));   break;
      case 5:  Serial.println(F("Cold"));   break;
      case 6:  Serial.println(F("Hot"));    break;
      default: Serial.println(F("Unknown")); break;
      }
      Serial.print(F("  - CHRG_STAT: "));
      Serial.println((int)charger.getCHRG_STAT());
    }
    break;
  default:
    break;
  }
}

// Diagnoses "VBUS reads voltage but IBUS reads no current" style symptoms.
// Call this only after checkChargeFaults() has run for this tick, since it
// reuses chargeFaultActive rather than re-reading the fault register.
void checkInputCurrentFaults() {
  // HIZ and the ILIM pin bit live in the Charge Current Limit register,
  // FORCE_ICO/FORCE_INDET/EN_ICO plus the IINDPM value live in the Input
  // Current Limit register, and EN_CHG lives in the Charger Control
  // Settings block -- none of these are kept fresh anywhere else in the
  // loop, so read them explicitly before checking their derived getters.
  // Without this, getEN_CHG() below would always read back the raw
  // register's zero-initialized cache (false/disabled) regardless of what
  // setEN_CHG() actually wrote to the chip.
  charger.readChargeCurrentLimitReg();
  charger.readInputCurrentLimitReg();
  charger.readChargeStatusReg();
  charger.readChargeControlSettingsReg();
  // NTC/TS protection can silently suspend charging independent of the
  // VBUS_OVP/TSHUT/TMR bits checkChargeFaults() watches, and VBAT is the
  // fastest way to tell "no battery attached" apart from a real fault -
  // both are only relevant to the unexplainedNotCharging case below, but
  // they're cheap single/double-byte reads so just keep them fresh here too.
  charger.readNtcStatusReg();
  charger.readADCVbatReg();

  bool hiz = charger.getHIZMode();
  bool chargeDisabled = !charger.getEN_CHG();
  uint8_t chrgStat = charger.getCHRG_STAT();
  // PG_STAT rather than VBUS_STAT: with AUTO_INDET_EN=0 the D+/D- detection
  // that fills in VBUS_STAT is bypassed, but PG still reports a good input.
  bool noSourceDetected = !charger.getPG_STAT();
  bool ilimPinMode = charger.getILIMPinFunction();
  float inputCurrentLimit = charger.getInputCurrentLimit();
  uint8_t ntcStatus = charger.getNTCStatus();
  float vbat = charger.getADC_VBAT();

  // Not charging despite a detected source, enabled charging, and no active
  // fault is the "everything looks configured right but current is still
  // zero" case -- usually points at no battery to sink current into, an
  // open BATFET/protection FET, or a TS/thermistor condition holding off
  // charging via JEITA/NTC protection.
  bool unexplainedNotCharging = (chrgStat == 0) && !noSourceDetected &&
                                !chargeDisabled && !hiz && !chargeFaultActive;

  bool newIssue = hiz || chargeDisabled || noSourceDetected ||
                  chargeFaultActive || unexplainedNotCharging;

  if (newIssue && !inputCurrentIssueActive) {
    inputCurrentIssueActive = true;
#ifdef SERIAL_DEBUG
    Serial.println(F("!!! NO INPUT CURRENT - possible causes: !!!"));
    if (hiz)
      Serial.println(F("  - HIZ mode is enabled, input is disconnected"));
    if (chargeDisabled)
      Serial.println(F("  - EN_CHG is disabled"));
    if (noSourceDetected)
      Serial.println(F("  - PG_STAT reports no good input source"));
    if (chargeFaultActive)
      Serial.println(F("  - Charge fault is active (see checkChargeFaults)"));
    Serial.print(F("  - Input current limit register is set to "));
    Serial.print(inputCurrentLimit);
    Serial.println(F(" A"));
    if (ilimPinMode) {
      Serial.println(F("  - ILIM pin also active (R8 500 ohm, ~2.2 A max)"));
    }
    if (unexplainedNotCharging) {
      Serial.print(F("  - Source detected, charging enabled, no fault, but "
                     "CHRG_STAT reports not charging: VBAT="));
      Serial.print(vbat);
      Serial.print(F("V, NTC_STATUS=0x"));
      Serial.println(ntcStatus, HEX);
      if (vbat < 1.0f) {
        Serial.println(
            F("    VBAT reads near 0V - check that a battery is actually "
              "connected and the BATFET/pack protection FET is closed"));
      }
      if (ntcStatus != 0) {
        Serial.println(F("    NTC_STATUS is nonzero - TS pin reports an "
                         "out-of-range/open/shorted thermistor condition "
                         "suspending charge"));
      }
    }
#endif
  } else if (!newIssue && inputCurrentIssueActive) {
    inputCurrentIssueActive = false;
#ifdef SERIAL_DEBUG
    Serial.println(F("Input current conditions look normal again"));
#endif
  }
}

void readStatFields() {
  charger.readChargeStatusReg();
  charger.readNtcStatusReg();
  charger.readFaultStatusReg();
  // charger.readCellBalStatReg();
}

/******************************************************/
/******************* OLED SCREENS *********************/
/******************************************************/
#ifdef ENABLE_OLED
void drawPoweringOnScreen() {
  display.SSD1306_Clear();
  display.SSD1306_setFontSize(1);
  display.SSD1306_GotoXY(20, 28);
  display.SSD1306_Puts("Powering On...", SSD1306_COLOR_WHITE, SSD1306_OPAQUE);
  display.SSD1306_UpdateScreen();
}

void drawPoweringDownScreen() {
  display.SSD1306_Clear();
  display.SSD1306_setFontSize(1);
  display.SSD1306_GotoXY(14, 28);
  display.SSD1306_Puts("Powering Down...", SSD1306_COLOR_WHITE, SSD1306_OPAQUE);
  display.SSD1306_UpdateScreen();
}

void drawPrimaryScreen() {
  // char title[] = "Battery Monitor";
  char percentageStr[5];
  char voltageStr[8];
  char currentStr[8];
  float ext1PowerCalc, ext2PowerCalc;

  char voltageExt1Str[8], voltageExt2Str[8];
  char currentExt1Str[8], currentExt2Str[8];
  char line[20];

  if (displayExtPower) {
    ext1PowerCalc = ext1Voltage * ext1Current;
    ext2PowerCalc = ext2Voltage * ext2Current;
  } else {
    ext1PowerCalc = ext1Current;
    ext2PowerCalc = ext2Current;
  }

  formatFloat2(batteryVoltage, voltageStr, sizeof(voltageStr));
  formatFloat2(ext1Voltage, voltageExt1Str, sizeof(voltageExt1Str));
  formatFloat2(ext2Voltage, voltageExt2Str, sizeof(voltageExt2Str));
  // formatFloat2(batteryCurrent, currentStr, sizeof(currentStr));
  formatFloat2(ext1PowerCalc, currentExt1Str, sizeof(currentExt1Str));
  formatFloat2(ext2PowerCalc, currentExt2Str, sizeof(currentExt2Str));

  // 2S pack: full-scale voltage is 2x the per-cell VREG limit programmed on the
  // charger
  float maxPackVoltage = 2.0f * charger.getCellVoltageLimit();
  int batteryPercent = 0;
  if (maxPackVoltage > 0.0f) {
    batteryPercent = (int)((batteryVoltage / maxPackVoltage) * 100.0f + 0.5f);
    if (batteryPercent < 0)
      batteryPercent = 0;
    else if (batteryPercent > 100)
      batteryPercent = 100;
  }
  snprintf(percentageStr, sizeof(percentageStr), "%d%%", batteryPercent);

  display.SSD1306_Clear();

  display.SSD1306_GotoXY(FIRST_COLUMN, TOP_ROW);
  // display.SSD1306_Puts(title, SSD1306_COLOR_WHITE, SSD1306_OPAQUE);
  // display.SSD1306_drawRoundRect(95, 0, 31, 15, 3, SSD1306_COLOR_WHITE);
  // display.SSD1306_DrawRectangle(126, 4, 2, 7, SSD1306_COLOR_WHITE);
  display.SSD1306_setFontSize(2);
  display.SSD1306_GotoXY(batteryPercent == 100 ? 0 : 8, 4);
  display.SSD1306_Puts(percentageStr, SSD1306_COLOR_WHITE, SSD1306_OPAQUE);

  display.SSD1306_setFontSize(1);
  snprintf(line, sizeof(line), "B: %sV", voltageStr);
  display.SSD1306_GotoXY(FIRST_COLUMN, 30);
  display.SSD1306_Puts(line, SSD1306_COLOR_WHITE, SSD1306_OPAQUE);

  formatFloat2(sysCurrent, currentStr, sizeof(currentStr));
  snprintf(line, sizeof(line), "I: %sA", currentStr);
  display.SSD1306_GotoXY(FIRST_COLUMN, 45);
  display.SSD1306_Puts(line, SSD1306_COLOR_WHITE, SSD1306_OPAQUE);

  /* External Voltages and Currents */
  snprintf(line, sizeof(line), "V1: %sV", voltageExt1Str);
  display.SSD1306_GotoXY(MIDDLE_COLUMN, TOP_ROW);
  display.SSD1306_Puts(line, SSD1306_COLOR_WHITE, SSD1306_OPAQUE);
  if (displayExtPower)
    snprintf(line, sizeof(line), "P1: %sW", currentExt1Str);
  else
    snprintf(line, sizeof(line), " I1: %sA", currentExt1Str);
  display.SSD1306_GotoXY(MIDDLE_COLUMN, TOP_ROW + 15);
  display.SSD1306_Puts(line, SSD1306_COLOR_WHITE, SSD1306_OPAQUE);

  snprintf(line, sizeof(line), "V2: %sV", voltageExt2Str);
  display.SSD1306_GotoXY(MIDDLE_COLUMN, TOP_ROW + 30);
  display.SSD1306_Puts(line, SSD1306_COLOR_WHITE, SSD1306_OPAQUE);
  if (displayExtPower)
    snprintf(line, sizeof(line), "P2: %sW", currentExt2Str);
  else
    snprintf(line, sizeof(line), " I2: %sA", currentExt2Str);
  display.SSD1306_GotoXY(MIDDLE_COLUMN, TOP_ROW + 45);
  display.SSD1306_Puts(line, SSD1306_COLOR_WHITE, SSD1306_OPAQUE);

  display.SSD1306_UpdateScreen();
}
#endif
