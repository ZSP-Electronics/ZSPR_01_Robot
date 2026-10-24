#pragma once

#include "ui/UiPage.h"

// Example app: backlight slider and buzzer switch inside a scrolling list.
// Copy this class to start a new app, then register it with UiManager::addApp().
class SettingsApp : public App
{
public:
    const char *name() const override { return "Settings"; }
    const char *icon() const override { return LV_SYMBOL_SETTINGS; }

protected:
    void onCreate(lv_obj_t *root) override;
    void onDestroy() override { list_ = nullptr; }

private:
    static void backlightCb(lv_event_t *e);
    static void buzzerCb(lv_event_t *e);

    lv_obj_t *list_ = nullptr;
};
