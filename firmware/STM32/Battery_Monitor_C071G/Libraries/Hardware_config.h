/*
 * Hardware_config.h
 * Created on: May 20, 2026
 */

#ifndef HARDWARE_CONFIG_H_
#define HARDWARE_CONFIG_H_

#include <string>              // std::string
#include <cstring>             // strlen()
typedef std::string String;
inline String F(String s) { return s; };

#include <main.h>

#define VERSION				0x0001

#define button_middle GPIOA, GPIO_PIN_1
#define power_good GPIOA, GPIO_PIN_2
#define pwr_activation GPIOA, GPIO_PIN_3
#define button_left GPIOA, GPIO_PIN_4
#define battery_int GPIOA, GPIO_PIN_5
// #define battery_int_pin GPIO_Pin GPIO_PIN_5
#define battery_cEnable GPIOA, GPIO_PIN_6
#define button_right GPIOA, GPIO_PIN_7
#define user_led GPIOB, GPIO_PIN_0
#define pwr_button_read GPIOB, GPIO_PIN_1

#endif /* HARDWARE_CONFIG_H_ */
