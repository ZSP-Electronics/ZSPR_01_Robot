#include <Arduino.h>
#include "HAL_Robot_Module.h"

HAL_Robot_Module robot;

void setup()
{
  // Serial.setDebugOutput(true);
  robot.begin();
}

void loop()
{
  robot.update();
}
