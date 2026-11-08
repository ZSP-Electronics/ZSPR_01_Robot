#include <Arduino.h>
#include "HAL_Robot_Module.h"
#include "Robot_Procedure.h"
#include "UI/UI_Task.h"

HAL_Robot_Module robot;
// The robot procedure is fed by the HAL (its modules) and runs in its own task.
Robot_Procedure procedure(robot);
UI_Task ui(robot, procedure);

void setup()
{
  robot.init();      // hardware: pins, buses, modules and their tasks
  procedure.begin(); // robot control: loads calibration/limits, starts the robot task
  ui.start();
}

void loop()
{
  robot.update();
}
