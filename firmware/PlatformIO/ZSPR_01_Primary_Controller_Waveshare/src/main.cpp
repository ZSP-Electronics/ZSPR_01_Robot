#include <Arduino.h>

#include "I2C_Driver.h"
#include "TCA9554PWR.h"
#include "Touch_GT911.h"
#include "RGBPanel.h"

#include "panel_config.h"
#include "ui/LvglPort.h"
#include "ui/UiManager.h"
#include "ui/FaceHome.h"
#include "ui/LauncherMenu.h"
#include "ui/apps/SettingsApp.h"

static RGBPanel panel;
static LvglPort lvgl;

static UiManager ui;
static FaceHome face(ui);
static LauncherMenu launcher(ui);

// One object per app; register each with ui.addApp() in setup().
static SettingsApp settingsApp;

void setup()
{
  Serial.begin(115200);

  I2C_Init();
  Wire.setClock(400000);
  TCA9554PWR_Init(0x00);    // LCD reset/CS, touch reset and buzzer live on the expander
  Set_EXIO(EXIO_PIN8, Low); // buzzer off

  disp_lcd_panel_config_t panel_config = makePanelConfig();
  if (!panel.begin(&panel_config))
  {
    Serial.println("[main] panel init failed");
    for (;;)
      delay(1000);
  }
  Touch_Init();

  if (!lvgl.begin(panel))
  {
    Serial.println("[main] LVGL init failed");
    for (;;)
      delay(1000);
  }

  LvglPort::lock();
  ui.addApp(settingsApp);
  ui.begin(face, launcher);
  // TODO: build the robot face inside face.faceContainer().
  LvglPort::unlock();

  Backlight_Init();
}

void loop()
{
  // The LVGL task owns core 1's UI work. Servo and sensor modules (Universal_Module::update())
  // are driven from here or from tasks pinned to core 0.
  delay(10);
}
