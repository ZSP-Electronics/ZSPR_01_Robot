#include <Arduino.h>
#include "HAL_Robot_Module.h"

HAL_Robot_Module robot;

void setup()
{
  robot.begin();
}

void loop()
{
  robot.update();
}
