#pragma once

#include <Arduino.h>
#include "hardware_config.h"
#include "I2C_Driver.h"
#include "TCA9554PWR.h"
#include "SimpleCLI.h"
#include "Touch_GT911.h"
#include "Gyro_QMI8658.h"
#include "Servo_Module.h"

inline Servo_Module servo;
inline SimpleCLI cli;

void HAL_Robot_Driver_Init();
void HAL_Robot_Driver_loop(void *parameter);
void CLI_task(void *parameter);