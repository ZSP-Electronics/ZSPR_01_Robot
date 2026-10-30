#include <Arduino.h>
#include "HAL_Robot_Module.h"
#include "app/UI_Task.h"

HAL_Robot_Module robot;
UI_Task ui(robot);

void setup()
{
  // Serial.setDebugOutput(true);
  robot.init();
  ui.start();
}

void loop()
{
  robot.update();
}
