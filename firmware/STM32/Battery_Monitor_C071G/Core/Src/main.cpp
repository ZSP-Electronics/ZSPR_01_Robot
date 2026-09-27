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
#include "i2c.h"
#include "usart.h"
#include "gpio.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Hardware_config.h"
#include "stm32fxxx.h"
#include "ssd1306.h"
#include <BQ25883.h>
#include "DialogBold10.h"
#include "Battery_Link.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// I2C_HandleTypeDef hi2c1;
void update_button_states(void);
void readBatteryStatus(void);
bool estimateSystemLoadCurrent(float* currentAmps);
static void formatFloat2(float value, char* buf, size_t bufSize);
void serviceWatchdog(void);
void checkChargeFaults(void);
void checkInputCurrentFaults(void);
void readStatFields(void);
void drawPrimaryScreen(void);

#ifdef DEBUG
void printFirstFiveRegs(void);
void printAllFields(void);
void printBatteryStatus(void);
#endif
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// HardwareSerial Serial(&huart1);
BQ25883 charger = BQ25883();
STM32_SSD1306 display;
Battery_Link batteryLink;

/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
bool battery_int_state = false;
bool left_button_state, middle_button_state, right_button_state = false;
bool previous_left_button_state, previous_middle_button_state, previous_right_button_state = false;
bool battery_pwr_good_state = false;
// uint8_t rxData[UART_BUFFER_SIZE];

//global variables for first five regsiters
float cellVoltageLimit = 0.0;

float chargeCurrentLimit = 0.0;
bool hizMode = true;
bool iLimPin  = false;

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
float ext1Voltage = 0.0;
float ext2Voltage = 0.0;
float ext1Current = 0.0;
float ext2Current = 0.0;
uint32_t lastBatteryReadTick = 0;

bool chargeFaultActive = false;
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
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
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
  #ifdef DEBUG
  Serial.println("Hello World!");
  #endif

  display.SSD1306_begin(BOARD_I2C1, SSD1306_I2C_ADDR);
  display.SSD1306_setFont(&Dialog_bold_10);
  display.SSD1306_setFontSize(1);

  if(charger.begin(BOARD_I2C1)){
    #ifdef DEBUG
    Serial.println(F("BQ25883 is working..."));
    #endif
    charger.setADC_EN(true);        //enable ADC
    charger.setADC_ONE_SHOT(false); //continuous conversion mode so VBAT/ICHG stay up to date
    // ILIM is grounded directly (no resistor) on this board, which the IC reads as the
    // minimum input current tier; since the effective limit is min(ILIM pin, IINDPM
    // register), that floor silently overrides IINDPM. Disable the pin function so the
    // register is authoritative - see setInputCurrentLimit() for the actual limit.
    charger.setEN_CHG(true);
    charger.setHIZMode(false);          //HIZ already defaults off at power-on-reset, set explicitly so the input path isn't left implicit
    charger.setILIMPinFunction(false);
    charger.setInputCurrentLimit(0.5);  //set IINDPM to 0.5A, which is the actual limit on this board
    // ICHG otherwise sits at its 1.5A power-on default (never set elsewhere), which is
    // higher than IINDPM and therefore misleading to read back - align it to the same
    // 500mA ceiling so getChargeCurrentLimit() reflects what can actually flow.
    charger.setChargeCurrentLimit(0.5);
    // charger.setADC_SAMPLE_SPEED(0b00); //fastest conversion speed
    // charger.readChargeCurrentLimitReg(); //cache ICHG so getChargeCurrentLimit()
    charger.readCellVoltageLimitReg(); //cache VREG so getCellVoltageLimit() reflects the actual charger setting, not a zeroed default
  }
  else{
    // If BQ25883 is not working, blink the LED to indicate an error
    #ifdef DEBUG
    Serial.println(F("BQ25883 is NOT working.  Check your wires..."));
    #endif
    while(1){
      Board_GPIO_TogglePin(user_led);
      HAL_Delay(500);
    }
  }

  drawPrimaryScreen();

  // getFirstFiveRegs();
  // printFirstFiveRegs();

  // printAllFields();

  
  
  /* USER CODE END 2 */
  
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    // Service the Primary Controller link: answer REQUEST_VOLTAGE frames and
    // latch any SEND_CURRENT_POWER rail data pushed from the Primary.
    batteryLink.poll();

    if(batteryLink.has_current_power()){
      const Battery_Link::RailReadings &r = batteryLink.rails();
      ext1Voltage = r.sysBus_mV   / 1000.0f;
      ext1Current = r.sysCurrent_mA / 1000.0f;
      ext2Voltage = r.motorBus_mV / 1000.0f;
      ext2Current = r.motorCurrent_mA / 1000.0f;
    }

    update_button_states();

    if(HAL_GetTick() - lastBatteryReadTick >= 1000){
      lastBatteryReadTick = HAL_GetTick();
      HAL_GPIO_TogglePin(user_led);
      serviceWatchdog();
      checkChargeFaults();
      checkInputCurrentFaults();
      readBatteryStatus();
      // printBatteryStatus();
      drawPrimaryScreen();
    }

    /* USER CODE END WHILE */
    // Serial.println("Hello World!");
    // printf("Hello World!\r\n");
    // HAL_UART_Receive(&huart1, rxData, 5, HAL_MAX_DELAY);
    // HAL_GPIO_TogglePin(user_led);
    // HAL_Delay(100);
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
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
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
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
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */


void update_button_states(void){
  bool left_but_temp = !Board_GPIO_ReadPin(button_left);
  bool middle_but_temp = !Board_GPIO_ReadPin(button_middle);
  bool right_but_temp = !Board_GPIO_ReadPin(button_right);
  battery_pwr_good_state = Board_GPIO_ReadPin(pwr_button_read);

  //Left Debounce
  if(left_but_temp)
  {
    if(!previous_left_button_state)
    {
      left_button_state = true;
      previous_left_button_state = left_button_state;
      Serial.println("Left Button Pressed");
    }
    else
      left_button_state = false;
  }
  else if(previous_left_button_state && !left_button_state)
  {
    previous_left_button_state = left_button_state;
  }
  else if(previous_left_button_state && left_button_state)
    left_button_state = false;

  //Middle Debounce
  if(middle_but_temp)
  {
    if(!previous_middle_button_state)
    {
      middle_button_state = true;
      previous_middle_button_state = middle_button_state;
      Serial.println("Middle Button Pressed");
    }
    else
      middle_button_state = false;
  }
  else if(previous_middle_button_state && !middle_button_state)
  {
    previous_middle_button_state = middle_button_state;
  }
  else if(previous_middle_button_state && middle_button_state)
    middle_button_state = false;

  //Right Debounce
  if(right_but_temp)
  {
    if(!previous_right_button_state)
    {
      right_button_state = true;
      previous_right_button_state = right_button_state;
      Serial.println("Right Button Pressed");
    }
    else
      right_button_state = false;
  }
  else if(previous_right_button_state && !right_button_state)
  {
    previous_right_button_state = right_button_state;
  }
  else if(previous_right_button_state && right_button_state)
    right_button_state = false;
}

void setFields(){
//  charger.setCellVoltageLimit(4.2);                           //cell voltage limit in Volt as a floating point, Range 3.4 - 4.6, default is 4.2V
//  charger.setChargeCurrentLimit(1.5);                         //charger current limit in Amp as a floating point, range 0.1 - 2.2, default is 1.5A
//  charger.setHIZMode(false);                                  //high impedance mode as boolean true or false, default is false
//  charger.setILIMPinFunction(true);                           //enable current limit pin as boolean true or false, default is true
//  charger.setInputVoltageLimit(4.3);                          //input voltage limit as floating point, Range 3.9 - 5.5, default is 4.3V
//  charger.setEN_VINDPM_RST(true);                             //enable VINDPM reset when adapter is plugged in as boolean true or false, default is true
//  charger.setEN_BAT_DISCHG(false);                            //enable battery discharge load as boolean true or false, default is false
//  charger.setPFM_OOA_DIS(false);                              //disable PFM Out-of-Audio mode as boolean true or false, default is false
//  charger.setInputCurrentLimit(3.0);                          //input current limit in Amps as floating point, range 0.5 - 3.3, default is 3.0A
//  charger.setFORCE_ICO(false);                                //force start input current optimizer as boolean true or false, default is false
//  charger.setFORCE_INDET(false);                              //force PSEL input detection as boolean true or false, default is false
//  charger.setEN_ICO(true);                                    //enable input current optimization algorithm control as boolean true or false, default is true
//  charger.setPrechargeCurrentLimit(0.15);                     //precharge current limit in Amp as floating point, range 0.05 - 0.8, default is 0.15;
//  charger.setTerminationCurrentLimit(0.15);                   //termination current limit in Amp as floating point, range 0.05 - 0.8, default 0.15A
//  charger.setEN_TERM(true);                                   //enable termination control as boolean true or false, default is true
//  charger.setSTAT_DIS(false);                                 //disable status pin as boolean true or false, default is false
//  charger.setWATCHDOG(1);                                     //watchdog timer setting as uint8_t, 0 is disable WD timer, 1 is 40s, 2 is 80s, 3 is 160s, incorrect values go to default value of 1
//  charger.setEN_TIMER(true);                                  //enable charging safety timer as boolean true or false, default is true
//  charger.setCHG_TIMER(2);                                    //fast charge timer setting as uint8_t, 0 is 5hrs, 1 is 8hrs, 2 is 12hrs, 3 is 20 hrs, incorrect values go to default value of 2
//  charger.setTMR2X_EN(true);                                  //enable slow safety timer by 2X during DPM or TREG as boolean true or false, default is true
//  charger.setAUTO_INDET_EN(true);                             //enable auto PSEL input detection as boolean true or false, default is true
//  charger.setT_REG_THRESH(3);                                 //thermal regulation threshold as uint8_t, 0 is 60C, 1 is 80C, 2 is 100C, 3 is 120C, incorrect values go to default value of 3
//  charger.setEN_CHG(true);                                    //enable charger as boolean true or false, default is true
//  charger.setCELLLOWV_THRESH(1);                              //cell low voltage threshold as uint8_t, 0 is 2.8V, 1 is 3.0V, incorrect values go to default value of 1
//  charger.setVCELL_RECHG_THRESH_OFF(1);                       //recharge threshold offset as uint8_t, 0 is 0.05V, 1 is 0.1V, 2 is 0.15V, 3 is 0.2V, incorrect values go to default value of 1
//  charger.setPFM_DIS(false);                                  //disable PFM Mode as boolean true or false, default is false
//  charger.setTOPOFF_TIMER(0);                                 //top-off timer setting as uint8_t, 0 is disable, 1 is 15min, 2 is 30min, 3 is 45min, incorrect values go to default value of 0
//  charger.setJEITA_VSET(1);                                   //JEITA high temp voltage setting as uint8_t, 0 is suspend charge, 1 is 8.0V, 2 is 8.3V, 3 is VREG unchanged, incorrect values go to default value of 1
//  charger.setJEITA_ISETH(1);                                  //JEITA high temp current setting as uint8_t, 0 is 40% ICHG, 1 is 100% ICHG, incorrect values go to default value of 1
//  charger.setJEITA_ISETC(1);                                  //JEITA low temp current setting as uint8_t, 0 is suspend charge, 1 is 20% ICHG, 2 is 40% ICHG, 3 is 100% ICH, incorrect values go to default value of 1
//  charger.setADC_DONE_MASK(false);                            //mask ADC conversion done from producing an INT pulse
//  charger.setIINDPM_MASK(false);                              //mask IINDPM regulation from producing an INT pulse
//  charger.setVINDPM_MASK(false);                              //mask VINDPM regulation from producing an INT pulse
//  charger.setT_REG_MASK(false);                               //mask IC temperature regulation from producing an INT pulse
//  charger.setWD_MASK(false);                                  //mask watchdog timer from producing an INT pulse
//  charger.setCHRG_MASK(false);                                //mask charge status from producing an INT pulse
//  charger.setPG_MASK(false);                                  //mask power good from producing an INT pulse
//  charger.setVBUS_MASK(false);                                //mask VBUS status from producing an INT pulse
//  charger.setTS_MASK(false);                                  //mask TS status from producing an INT pulse
//  charger.setICO_MASK(false);                                 //mask input current optimizer from producing an INT pulse
//  charger.setVBUS_OVP_MASK(false);                            //mask input over-voltage fault from producing an INT pulse
//  charger.setTSHUT_MASK(false);                               //mask thermal shutdown fault from producing an INT pulse
//  charger.setTMR_MASK(false);                                 //mask charge safety timer fault from producing an INT pulse
//  charger.setSNS_SHORT_MASK(false);                           //mask SNS short fault from producing an INT pulse
//  charger.setADC_EN(true);                                    //enable ADC control
//  charger.setADC_ONE_SHOT(false);                             //enable ADC one-shot mode
//  charger.setADC_SAMPLE_SPEED(0);                             //ADC sample speed as uint8_t, 0 is slowest with 15 bit resolution, 1 has 14 bit resolution, 2 has 13 bit resolution, 3 is the fastest with 12 bit resolution, incorrect values default to 0
//  charger.setIBUS_ADC_DIS(false);                             //disable ADC IBUS
//  charger.setICHG_ADC_DIS(false);                             //disable ADC ICHG
//  charger.setVBUS_ADC_DIS(false);                             //disable ADC VBUS
//  charger.setVBAT_ADC_DIS(false);                             //disable ADC VBAT
//  charger.setTS_ADC_DIS(false);                               //disbale ADC TS
//  charger.setVCELL_ADC_DIS(false);                            //disable ADC VCELL
//  charger.setTDIE_ADC_DIS(false);                             //disable ADC TDIE
//  charger.setVDIFF_END_OFFSET(1);                             //cell ballancing exit threshold as uint8_t, 0 throu 7, 0 is 0.03V, 7 is 0.1V, step is 0.01V, incorrect values go to the default value of 1
//  charger.setTCB_QUAL_INTERVAL(0);                            //interval between taking meassurments for cell balancing as uint8_t, 0 is 2min, 1 is 4 min, incorrect values go to default value of 0
//  charger.setTCB_ACTIVE(2);                                   //time interval to stop charging and discharging for cell voltage meassurments as uint8_t, 0 is 4s, 1 is 32s, 2 is 2min, 3 is 4min, incorrect values go to default value of 2
//  charger.setTSETTLE(2);                                      //delay between charge disable and voltage meassurment as uint8_t, 0 is 10ms, 1 is 100ms, 2 is 1s, 3 is 2s, incorrect values go to default value of 2
//  charger.setVQUAL_TH(15);                                    //threshold from cell balancing pre-qual to cell balancing qual. 0 is 40mV, 14 is 180mV, 10mV steps. 15 is disabled.  If incorrect value sets to default disabled
//  charger.setVDIFF_START(4);                                  //threshold from cell balancing qual to cell balancing active. 0 is 40mV, 15 is 190mV, 10mV steps.  If incorrect value goes to default of 4
//  charger.setCB_CHG_DIS(true);                                //disable charge for accurate cell balancing measurment
//  charger.setCB_AUTO_EN(true);                                //enable automatic cell balancing
//  charger.setQCBL_EN(false);                                  //turn on QCBH to discharge top cell
//  charger.setQCBH_EN(false);                                  //turn on QCBL to discharge bottom cell
//  charger.setCB_MASK(false);                                  //mask cell balancing INT pulse
//  charger.setHS_CV_MASK(false);                               //mask high side cell balancing FET in CV mode INT pulse
//  charger.setLS_CV_MASK(false);                               //mask low side cell balancing FET in CV mode INT pulse
//  charger.setHS_OV_MASK(false);                               //mask high cell in over voltage INT pulse
//  charger.setLS_OV_MASK(false);                               //mask low cell in over voltage INT pulse
//  charger.setCB_OC_MASK(false);                               //mask cell balance over-current protection active INT pulse

//  charger.wdReset();                                          //reset watchdog timer
//  charger.registerReset();                                    //resets all the registers
}

#ifdef DEBUG
void printAllFields(){
  charger.pollAllRegs();

  //Cell Voltage Limit
  Serial.print(F("Cell Voltage Limit: ")); Serial.println(charger.getCellVoltageLimit());

  //Charge Current Limit
  Serial.print(F("Charge Current Limit: ")); Serial.println(charger.getChargeCurrentLimit());
  Serial.print(F("hizMode: ")); Serial.println(charger.getHIZMode());
  Serial.print(F("iLimPin: ")); Serial.println(charger.getILIMPinFunction());

  //Input Voltage Limit
  Serial.print(F("Input Voltage Limit: ")); Serial.println(charger.getInputVoltageLimit());
  Serial.print(F("vInDpmRst: ")); Serial.println(charger.getEN_VINDPM_RST());
  Serial.print(F("batDischg: ")); Serial.println(charger.getEN_BAT_DISCHG());
  Serial.print(F("pfmOoa: ")); Serial.println(charger.getPFM_OOA_DIS());

  //Input Current Limit
  Serial.print(F("Input Current Limit: ")); Serial.println(charger.getInputCurrentLimit());
  Serial.print(F("forceIco: ")); Serial.println(charger.getFORCE_ICO());
  Serial.print(F("forceIndet: ")); Serial.println(charger.getFORCE_INDET());
  Serial.print(F("enIco: ")); Serial.println(charger.getEN_ICO());

  //Pre/Term Current Limit
  Serial.print(F("Precharge Current Limit: ")); Serial.println(charger.getPrechargeCurrentLimit());
  Serial.print(F("Termination Current Limit: ")); Serial.println(charger.getTerminationCurrentLimit());

  //charge control settings
  Serial.print(F("Enable Termination: ")); Serial.println(charger.getEN_TERM());
  Serial.print(F("Status Disable: ")); Serial.println(charger.getSTAT_DIS());
  Serial.print(F("Watchdog: ")); Serial.println(charger.getWATCHDOG());
  Serial.print(F("Enable Timer: ")); Serial.println(charger.getEN_TIMER());
  Serial.print(F("Charge Timer: ")); Serial.println(charger.getCHG_TIMER());
  Serial.print(F("Timer 2X Enable: ")); Serial.println(charger.getTMR2X_EN());
  Serial.print(F("Auto Input Detect Enable: ")); Serial.println(charger.getAUTO_INDET_EN());
  Serial.print(F("Thermal Regulation Threshold: ")); Serial.println(charger.getT_REG_THRESH());
  Serial.print(F("Enable Charging: ")); Serial.println(charger.getEN_CHG());
  Serial.print(F("Cell Low Volt Threshold: ")); Serial.println(charger.getCELLLOWV_THRESH());
  Serial.print(F("Volt Cell Recharge Threshold Offset: ")); Serial.println(charger.getVCELL_RECHG_THRESH_OFF());
  Serial.print(F("PFM Disable: ")); Serial.println(charger.getPFM_DIS());
  Serial.print(F("Watchdog Reset: ")); Serial.println(charger.getWD_RST());
  Serial.print(F("Top-Off Timer: ")); Serial.println(charger.getTOPOFF_TIMER());
  Serial.print(F("JEITA Volt Set: ")); Serial.println(charger.getJEITA_VSET());
  Serial.print(F("JEITA Current Set HOT: ")); Serial.println(charger.getJEITA_ISETH());
  Serial.print(F("JEITA Current Set COLD: ")); Serial.println(charger.getJEITA_ISETC());

  //ICO Current Limit
  Serial.print(F("ICO Current Limit: ")); Serial.println(charger.getICOCurrentLimit());

  //Charge Status  
  Serial.print(F("Current In DPM Status: ")); Serial.println(charger.getIINDPM_STAT());
  Serial.print(F("Voltage In DPM Status: ")); Serial.println(charger.getVINDPM_STAT());
  Serial.print(F("Thermal Regulation Status: ")); Serial.println(charger.getTREG_STAT());
  Serial.print(F("Watchdog Status: ")); Serial.println(charger.getWD_STAT());
  Serial.print(F("Charge Status: ")); Serial.println(charger.getCHRG_STAT());
  Serial.print(F("Power Good Status: ")); Serial.println(charger.getPG_STAT());
  Serial.print(F("VBUS Status: ")); Serial.println(charger.getVBUS_STAT());
  Serial.print(F("ICO Status: ")); Serial.println(charger.getICO_STAT());

  //NTC Status
  Serial.print(F("NTC Status: ")); Serial.println(charger.getNTCStatus());

  //Fault Status
  Serial.print(F("VBUS OVP Fault Status: ")); Serial.println(charger.getVBUS_OVP_STAT());
  Serial.print(F("TSHUT Fault Status: ")); Serial.println(charger.getTSHUT_STAT());
  Serial.print(F("Timer Status: ")); Serial.println(charger.getTMR_STAT());

  //Charge Flags
  Serial.print(F("Current In DPM Flag: ")); Serial.println(charger.getIINDPM_FLAG());
  Serial.print(F("Voltage In DPM Flag: ")); Serial.println(charger.getVINDPM_FLAG());
  Serial.print(F("Thermal Regulation Flag: ")); Serial.println(charger.getTREG_FLAG());
  Serial.print(F("Watchdog Flag: ")); Serial.println(charger.getWD_FLAG());
  Serial.print(F("Charge Flag: ")); Serial.println(charger.getCHRG_FLAG());
  Serial.print(F("Power Good Flag: ")); Serial.println(charger.getPG_FLAG());
  Serial.print(F("VBUS Flag: ")); Serial.println(charger.getVBUS_FLAG());
  Serial.print(F("TS Flag: ")); Serial.println(charger.getTS_FLAG());
  Serial.print(F("ICO Flag: ")); Serial.println(charger.getICO_FLAG());

  //Fault Flags
  Serial.print(F("VBUS OVP Fault Flag: ")); Serial.println(charger.getVBUS_OVP_FLAG());
  Serial.print(F("TSHUT Fault Flag: ")); Serial.println(charger.getTSHUT_FLAG());
  Serial.print(F("Timer Fault Flag: ")); Serial.println(charger.getTMR_FLAG());

  //Charger INT Masks
  Serial.print(F("ADC Done Mask: ")); Serial.println(charger.getADC_DONE_MASK());
  Serial.print(F("IINDPM Mask: ")); Serial.println(charger.getIINDPM_MASK());
  Serial.print(F("VINDOM Mask: ")); Serial.println(charger.getVINDPM_MASK());
  Serial.print(F("T_REG Mask: ")); Serial.println(charger.getT_REG_MASK());
  Serial.print(F("Watchdog Mask: ")); Serial.println(charger.getWD_MASK());
  Serial.print(F("Charge Mask: ")); Serial.println(charger.getCHRG_MASK());
  Serial.print(F("Power Good Mask: ")); Serial.println(charger.getPG_MASK());
  Serial.print(F("VBUS Mask: ")); Serial.println(charger.getVBUS_MASK());
  Serial.print(F("TS Mask: ")); Serial.println(charger.getTS_MASK());
  Serial.print(F("ICO Mask: ")); Serial.println(charger.getICO_MASK());

  //Fault INT Masks
  Serial.print(F("VBUS OVP Mask: ")); Serial.println(charger.getVBUS_OVP_MASK());
  Serial.print(F("TSHUT Mask: ")); Serial.println(charger.getTSHUT_MASK());
  Serial.print(F("Timer Mask: ")); Serial.println(charger.getTMR_MASK());
  Serial.print(F("SNS Short Mask: ")); Serial.println(charger.getSNS_SHORT_MASK());

  //ADC Control Settings
  Serial.print(F("ADC Enable: ")); Serial.println(charger.getADC_EN());
  Serial.print(F("ADC One Shot: ")); Serial.println(charger.getADC_ONE_SHOT());
  Serial.print(F("ADC Sample Speed: ")); Serial.println(charger.getADC_SAMPLE_SPEED());

  //ADC Function Disable Settings
  Serial.print(F(": ")); Serial.println(charger.getIBUS_ADC_DIS());
  Serial.print(F(": ")); Serial.println(charger.getICHG_ADC_DIS());
  Serial.print(F(": ")); Serial.println(charger.getVBUS_ADC_DIS());
  Serial.print(F(": ")); Serial.println(charger.getVBAT_ADC_DIS());
  Serial.print(F(": ")); Serial.println(charger.getTS_ADC_DIS());
  Serial.print(F(": ")); Serial.println(charger.getVCELL_ADC_DIS());
  Serial.print(F(": ")); Serial.println(charger.getTDIE_ADC_DIS());

  //ADC
  Serial.print(F("ADC IBUS: ")); Serial.println(charger.getADC_IBUS());
  Serial.print(F("ADC ICHG: ")); Serial.println(charger.getADC_ICHG());
  Serial.print(F("ADC VBUS: ")); Serial.println(charger.getADC_VBUS());
  Serial.print(F("ADC VBAT: ")); Serial.println(charger.getADC_VBAT());
  Serial.print(F("ADC Volt Top Cell: ")); Serial.println(charger.getADC_VCELLTOP());
  Serial.print(F("ADC TS: ")); Serial.println(charger.getADC_TS());
  Serial.print(F("ADC TDIE: ")); Serial.println(charger.getADC_TDIE());
  Serial.print(F("ADC Volt Bottom Cell: ")); Serial.println(charger.getADC_VCELLBOT());

  //Part INFO
  Serial.print(F("Part Number: ")); Serial.println(charger.getPartNumber());
  Serial.print(F("Device Revision: ")); Serial.println(charger.getDevRev());

  //Cell Balance Control Settings
  // Serial.print(F("VDIFF END Offset: ")); Serial.println(charger.getVDIFF_END_OFFSET());
  // Serial.print(F("TCB Qual Interval: ")); Serial.println(charger.getTCB_QUAL_INTERVAL());
  // Serial.print(F("TCB Active: ")); Serial.println(charger.getTCB_ACTIVE());
  // Serial.print(F("T Settle: ")); Serial.println(charger.getTSETTLE());
  // Serial.print(F(":V Qual Threshold ")); Serial.println(charger.getVQUAL_TH());
  // Serial.print(F("VDIFF Start: ")); Serial.println(charger.getVDIFF_START());

  // //Cell Balance Status
  // Serial.print(F(": ")); Serial.println(charger.getCB_CHG_DIS());
  // Serial.print(F(": ")); Serial.println(charger.getCB_AUTO_EN());
  // Serial.print(F(": ")); Serial.println(charger.getCB_STAT());
  // Serial.print(F(": ")); Serial.println(charger.getHS_CV_STAT());
  // Serial.print(F(": ")); Serial.println(charger.getLS_CV_STAT());
  // Serial.print(F(": ")); Serial.println(charger.getHS_OV_STAT());
  // Serial.print(F(": ")); Serial.println(charger.getLS_OV_STAT());
  // Serial.print(F(": ")); Serial.println(charger.getCB_OC_STAT());

  // //Cell Balance Flags
  // Serial.print(F(": ")); Serial.println(charger.getQCBH_EN());
  // Serial.print(F(": ")); Serial.println(charger.getQCBL_EN());
  // Serial.print(F(": ")); Serial.println(charger.getCB_FLAG());
  // Serial.print(F(": ")); Serial.println(charger.getHS_CV_FLAG());
  // Serial.print(F(": ")); Serial.println(charger.getLS_CV_FLAG());
  // Serial.print(F(": ")); Serial.println(charger.getHS_OV_FLAG());
  // Serial.print(F(": ")); Serial.println(charger.getLS_OV_FLAG());
  // Serial.print(F(": ")); Serial.println(charger.getCB_OC_FLAG());

  // //Cell Balance INT Mask
  // Serial.print(F(": ")); Serial.println(charger.getCB_MASK());
  // Serial.print(F(": ")); Serial.println(charger.getHS_CV_MASK());
  // Serial.print(F(": ")); Serial.println(charger.getLS_CV_MASK());
  // Serial.print(F(": ")); Serial.println(charger.getHS_OV_MASK());
  // Serial.print(F(": ")); Serial.println(charger.getLS_OV_MASK());
  // Serial.print(F(": ")); Serial.println(charger.getCB_OC_MASK());
}
#endif

void readADCRegs(){
  charger.readADCIbusReg();
  charger.readADCIchgReg();
  charger.readADCVbusReg();
  charger.readADCVbatReg();
  charger.readADCVCellTopReg();
  charger.readADCTsReg();
  charger.readADCTDieReg();
  charger.readADCVCellBotReg();
}

void printADCRegs(){
  Serial.println();
  Serial.print(F("ADC IBUS: ")); Serial.println(charger.getADC_IBUS());
  Serial.print(F("ADC ICHG: ")); Serial.println(charger.getADC_ICHG());
  Serial.print(F("ADC VBUS: ")); Serial.println(charger.getADC_VBUS());
  Serial.print(F("ADC VBAT: ")); Serial.println(charger.getADC_VBAT());
  Serial.print(F("ADC CELL TOP: ")); Serial.println(charger.getADC_VCELLTOP());
  Serial.print(F("ADC CELL BOT: ")); Serial.println(charger.getADC_VCELLBOT());
}

void readBatteryStatus(){
  charger.readADCVbatReg();
  batteryVoltage = charger.getADC_VBAT();

  // Cache the pack voltage (mV) the link hands back on REQUEST_VOLTAGE.
  batteryLink.set_battery_voltage_mV((uint16_t)(batteryVoltage * 1000.0f + 0.5f));

  // charger.readADCIbusReg();
  // batteryCurrent = charger.getADC_IBUS();
}

void printBatteryStatus(){
  Serial.print(F("Charge Voltage: ")); Serial.print(charger.readADCVbusReg()); Serial.println(F(" mV"));
  Serial.print(F("Charge Current: ")); Serial.print(charger.readADCIbusReg()); Serial.println(F(" mA"));
}

// Formats to 2 decimal places using integer math only - this MCU's nano.specs libc
// has no float printf support, so "%f" would silently print nothing.
static void formatFloat2(float value, char* buf, size_t bufSize){
  bool negative = value < 0.0f;
  if(negative) value = -value;
  value += 0.005f;

  unsigned long wholePart = (unsigned long)value;
  unsigned int fracPart = (unsigned int)((value - (float)wholePart) * 100.0f);

  snprintf(buf, bufSize, "%s%lu.%02u", negative ? "-" : "", wholePart, fracPart);
}

void serviceWatchdog(){
  //WD_RST must be re-written before the 40s watchdog window (see setWATCHDOG) expires,
  //otherwise the IC silently reverts every register to its power-on default (charging disabled, ADC off, etc.)
  charger.wdReset();

  if(charger.getWD_STAT()){
    #ifdef DEBUG
    Serial.println(F("!!! WATCHDOG EXPIRED - charger settings reverted to defaults, re-applying config !!!"));
    #endif
    charger.setADC_EN(true);
    charger.setADC_ONE_SHOT(false);
    // EN_ILIM resets to 1 (enabled) at power-on-default, which would silently
    // re-introduce the grounded-ILIM current-limit floor - reapply the same
    // override as setup().
    charger.setILIMPinFunction(false);
    // IINDPM/ICHG also reset to their power-on defaults (3.0A/1.5A), well above
    // this board's 500mA input ceiling - reapply the same limits as setup().
    charger.setInputCurrentLimit(0.5);
    charger.setChargeCurrentLimit(0.5);
  }
}

void checkChargeFaults(){
  charger.readFaultStatusReg();

  bool vbusOvp = charger.getVBUS_OVP_STAT();
  bool tshut = charger.getTSHUT_STAT();
  bool tmrFault = charger.getTMR_STAT();
  bool newFault = vbusOvp || tshut || tmrFault;

  if(newFault && !chargeFaultActive){
    chargeFaultActive = true;
    charger.setEN_CHG(false);
    #ifdef DEBUG
    Serial.println(F("!!! CHARGE FAULT DETECTED - CHARGING DISABLED !!!"));
    if(vbusOvp)   Serial.println(F("  - VBUS Overvoltage"));
    if(tshut)     Serial.println(F("  - Thermal Shutdown"));
    if(tmrFault)  Serial.println(F("  - Safety Timer Expired"));
    #endif
  }
  else if(!newFault && chargeFaultActive){
    chargeFaultActive = false;
    charger.setEN_CHG(true);
    #ifdef DEBUG
    Serial.println(F("Charge fault cleared - charging re-enabled"));
    #endif
  }
}

// Diagnoses "VBUS reads voltage but IBUS reads no current" style symptoms.
// Call this only after checkChargeFaults() has run for this tick, since it
// reuses chargeFaultActive rather than re-reading the fault register.
void checkInputCurrentFaults(){
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
  uint8_t vbusStat = charger.getVBUS_STAT();
  uint8_t chrgStat = charger.getCHRG_STAT();
  bool noSourceDetected = (vbusStat == 0);
  bool ilimPinMode = charger.getILIMPinFunction();
  float inputCurrentLimit = charger.getInputCurrentLimit();
  uint8_t ntcStatus = charger.getNTCStatus();
  float vbat = charger.getADC_VBAT();

  // Not charging despite a detected source, enabled charging, and no active
  // fault is the "everything looks configured right but current is still
  // zero" case -- usually points at no battery to sink current into, an
  // open BATFET/protection FET, or a TS/thermistor condition holding off
  // charging via JEITA/NTC protection.
  bool unexplainedNotCharging = (chrgStat == 0) && !noSourceDetected && !chargeDisabled && !hiz && !chargeFaultActive;

  bool newIssue = hiz || chargeDisabled || noSourceDetected || chargeFaultActive || ilimPinMode || unexplainedNotCharging;

  if(newIssue && !inputCurrentIssueActive){
    inputCurrentIssueActive = true;
    #ifdef DEBUG
    Serial.println(F("!!! NO INPUT CURRENT - possible causes: !!!"));
    if(hiz)               Serial.println(F("  - HIZ mode is enabled, input is disconnected"));
    if(chargeDisabled)    Serial.println(F("  - EN_CHG is disabled"));
    if(noSourceDetected)  Serial.println(F("  - VBUS_STAT reports no input source detected"));
    if(chargeFaultActive) Serial.println(F("  - Charge fault is active (see checkChargeFaults)"));
    if(ilimPinMode){
      Serial.println(F("  - ILIM pin mode active - input current is set by the external ILIM resistor, not the IINDPM register; check that resistor"));
    }
    else{
      Serial.print(F("  - Input current limit register is set to "));
      Serial.print(inputCurrentLimit);
      Serial.println(F(" A"));
    }
    if(unexplainedNotCharging){
      Serial.print(F("  - Source detected, charging enabled, no fault, but CHRG_STAT reports not charging: VBAT="));
      Serial.print(vbat);
      Serial.print(F("V, NTC_STATUS=0x"));
      Serial.println(ntcStatus, HEX);
      if(vbat < 1.0f){
        Serial.println(F("    VBAT reads near 0V - check that a battery is actually connected and the BATFET/pack protection FET is closed"));
      }
      if(ntcStatus != 0){
        Serial.println(F("    NTC_STATUS is nonzero - TS pin reports an out-of-range/open/shorted thermistor condition suspending charge"));
      }
    }
    #endif
  }
  else if(!newIssue && inputCurrentIssueActive){
    inputCurrentIssueActive = false;
    #ifdef DEBUG
    Serial.println(F("Input current conditions look normal again"));
    #endif
  }
}

void readStatFields(){
  charger.readChargeStatusReg();
  charger.readNtcStatusReg();
  charger.readFaultStatusReg();
  // charger.readCellBalStatReg();
}

// void blinkFaults(){
//   charger.readFaultStatusReg();

//   if(charger.getVBUS_OVP_STAT()){
//     blinkNumber(1);
//   }
  
//   if(charger.getTSHUT_STAT()){
//     blinkNumber(2);
//   }

//   if(charger.getTMR_STAT()){
//     blinkNumber(3);
//   }

//   blinkFast();
// }

// void blinkFast(){
//   for(int i = 0; i < 10; i++){
//     0(ERROR_LED, LOW);
//     delay(100);
//     digitalWrite(ERROR_LED, HIGH);
//     delay(100);
//   }

//   digitalWrite(ERROR_LED, LOW);
// }

// void blinkNumber(int numberBlinks){
//   for(int j = 0; j < 2; j++){
//     for(int i = 0; i < numberBlinks; i++){
//       digitalWrite(ERROR_LED, LOW);
//       delay(1000);
//       digitalWrite(ERROR_LED, HIGH);
//       delay(1000);
//     }
//     digitalWrite(ERROR_LED, LOW);
//     delay(3000);
//   }
// }

/******************************************************/
/******************* OLED SCREENS *********************/
/******************************************************/
void drawPrimaryScreen(){
  // char title[] = "Battery Monitor";
  char percentageStr[5];
  char voltageStr[8];
  float ext1PowerCalc, ext2PowerCalc;

  char voltageExt1Str[8], voltageExt2Str[8];
  char currentExt1Str[8], currentExt2Str[8];
  char line[20];

  if(displayExtPower){
    ext1PowerCalc = ext1Voltage * ext1Current;
    ext2PowerCalc = ext2Voltage * ext2Current;
  }
  else
  {
    ext1PowerCalc = ext1Current;
    ext2PowerCalc = ext2Current;
  }

  formatFloat2(batteryVoltage, voltageStr, sizeof(voltageStr));
  formatFloat2(ext1Voltage, voltageExt1Str, sizeof(voltageExt1Str));
  formatFloat2(ext2Voltage, voltageExt2Str, sizeof(voltageExt2Str));
  // formatFloat2(batteryCurrent, currentStr, sizeof(currentStr));
  formatFloat2(ext1PowerCalc, currentExt1Str, sizeof(currentExt1Str));
  formatFloat2(ext2PowerCalc, currentExt2Str, sizeof(currentExt2Str));

  // 2S pack: full-scale voltage is 2x the per-cell VREG limit programmed on the charger
  float maxPackVoltage = 2.0f * charger.getCellVoltageLimit();
  int batteryPercent = 0;
  if(maxPackVoltage > 0.0f){
    batteryPercent = (int)((batteryVoltage / maxPackVoltage) * 100.0f + 0.5f);
    if(batteryPercent < 0) batteryPercent = 0;
    else if(batteryPercent > 100) batteryPercent = 100;
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
  display.SSD1306_GotoXY(FIRST_COLUMN, 40);
  display.SSD1306_Puts(line, SSD1306_COLOR_WHITE, SSD1306_OPAQUE);

  /* External Voltages and Currents */
  snprintf(line, sizeof(line), "V1: %sV", voltageExt1Str);
  display.SSD1306_GotoXY(MIDDLE_COLUMN, TOP_ROW);
  display.SSD1306_Puts(line, SSD1306_COLOR_WHITE, SSD1306_OPAQUE);
  if(displayExtPower)
    snprintf(line, sizeof(line), "P1: %sW", currentExt1Str);
  else
    snprintf(line, sizeof(line), " I1: %sA", currentExt1Str);
  display.SSD1306_GotoXY(MIDDLE_COLUMN, TOP_ROW + 15);
  display.SSD1306_Puts(line, SSD1306_COLOR_WHITE, SSD1306_OPAQUE);

  snprintf(line, sizeof(line), "V2: %sV", voltageExt2Str);
  display.SSD1306_GotoXY(MIDDLE_COLUMN, TOP_ROW + 30);
  display.SSD1306_Puts(line, SSD1306_COLOR_WHITE, SSD1306_OPAQUE);
  if(displayExtPower)
    snprintf(line, sizeof(line), "P2: %sW", currentExt2Str);
  else
    snprintf(line, sizeof(line), " I2: %sA", currentExt2Str);
  display.SSD1306_GotoXY(MIDDLE_COLUMN, TOP_ROW + 45);
  display.SSD1306_Puts(line, SSD1306_COLOR_WHITE, SSD1306_OPAQUE);

  display.SSD1306_UpdateScreen();
}


