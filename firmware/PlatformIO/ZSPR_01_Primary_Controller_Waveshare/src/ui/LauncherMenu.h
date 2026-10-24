#pragma once

#include "UiPage.h"

class UiManager;

// Overlay with one round icon per registered app, spaced along a semicircle at the bottom edge.
// Swipe down or tap the dimmed background to close it.
class LauncherMenu : public Menu
{
public:
    explicit LauncherMenu(UiManager &ui) : ui_(ui) {}

protected:
    void onCreate(lv_obj_t *root) override;

private:
    static void gestureCb(lv_event_t *e);
    static void backgroundTapCb(lv_event_t *e);
    static void iconCb(lv_event_t *e);

    UiManager &ui_;
};
