#include <Arduino.h>

#include "RGBPanel.h"
#include "RGBRoboFace.h"
#include "hal/HAL_Robot_Driver.h"
#include "ui/LvglPort.h"
#include "ui/UiManager.h"
#include "ui/FaceHome.h"
#include "ui/LauncherMenu.h"
#include "ui/apps/SettingsApp.h"
#include "panel_config.h"

static RGBPanel panel;
static LvglPort lvgl;

static UiManager ui;
static FaceHome face(ui);
static RGBroboFace robotFace;
static LauncherMenu launcher(ui);

// One object per app; register each with ui.addApp() in setup().
static SettingsApp settingsApp;

void setup()
{
  Serial.begin(115200);

  HAL_Robot_Driver_Init();

  disp_lcd_panel_config_t panel_config = makePanelConfig();
  if (!panel.begin(&panel_config))
  {
    Serial.println("[main] panel init failed");
    for (;;)
      delay(1000);
  }

  if (!lvgl.begin(panel))
  {
    Serial.println("[main] LVGL init failed");
    for (;;)
      delay(1000);
  }

  LvglPort::lock();
  ui.addApp(settingsApp);
  ui.begin(face, launcher);
  robotFace.begin(face.faceContainer());
  robotFace.setEyeWidth(130);
  robotFace.setEyeHeight(160);
  robotFace.setEyeBorderradius(40);
  robotFace.setEyeSpacebetween(20);
  robotFace.setAutoblinker(true, 2, 4);
  robotFace.setCuriosity(true);
  robotFace.setPositionDistance(15);
  LvglPort::unlock();

  xTaskCreatePinnedToCore(HAL_Robot_Driver_loop, "Other Driver task", 4096, NULL, 3, NULL, 0);
}

void loop()
{
  // The LVGL task owns core 1's UI work. Servo and sensor modules (Universal_Module::update())
  // are driven from here or from tasks pinned to core 0.
  delay(10);
}
