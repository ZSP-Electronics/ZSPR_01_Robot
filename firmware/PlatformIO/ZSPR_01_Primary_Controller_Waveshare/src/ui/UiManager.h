#pragma once

#include <vector>
#include "UiPage.h"

// Owns navigation: the home page (face), the launcher overlay and the running app.
// Call everything with the LVGL lock held.
class UiManager
{
public:
    void addApp(App &app) { apps_.push_back(&app); }
    const std::vector<App *> &apps() const { return apps_; }

    // Opens `home` on the active screen and prepares `launcher` on the top layer.
    // Register apps before calling this (the launcher builds its icons from them).
    void begin(UiPage &home, Menu &launcher);

    void showLauncher();
    void hideLauncher();
    bool launcherVisible() const { return launcher_visible_; }

    void openApp(App &app);
    void closeApp();

private:
    static void backCb(lv_event_t *e);

    std::vector<App *> apps_;
    UiPage *home_ = nullptr;
    Menu *launcher_ = nullptr;
    App *running_ = nullptr;
    lv_obj_t *home_screen_ = nullptr;
    lv_obj_t *app_screen_ = nullptr;
    bool launcher_visible_ = false;
};
