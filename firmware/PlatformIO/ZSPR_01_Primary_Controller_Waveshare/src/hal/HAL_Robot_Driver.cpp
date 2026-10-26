#include "HAL_Robot_Driver.h"

void HAL_Robot_Driver_Init()
{
    I2C_Init();
    Wire.setClock(400000);
    delay(20);
    TCA9554PWR_Init(0x00);    // LCD reset/CS, touch reset and buzzer live on the expander
    Set_EXIO(EXIO_PIN8, Low); // buzzer off
    Touch_Init();
    QMI8658_Init();
    servo.setup();
}

void HAL_Robot_Driver_loop(void *)
{
    while(1) // a FreeRTOS task must never return
    {
        QMI8658_Loop();
        Touch_Loop();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void CLI_task(void *)
{
    // cli.addCommand(...) once, before the loop
    while(1)
    {
        while (Serial.available())
            cli.parse(Serial.readStringUntil('\n'));
        while (cli.available())
        {
            Command c = cli.getCommand();
            // dispatch by c.getName(); for face commands:
            //   LvglPort::lock(); robotFace.setEyeExpression(Happy); LvglPort::unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}