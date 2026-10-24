#pragma once

#include "UiPage.h"

class UiManager;

// Home page. Swipe up opens the launcher.
class FaceHome : public UiPage
{
public:
    explicit FaceHome(UiManager &ui) : ui_(ui) {}

    // Parent the robot face objects here. Children must have LV_OBJ_FLAG_GESTURE_BUBBLE
    // set or they swallow the swipe-up gesture.
    lv_obj_t *faceContainer() const { return root_; }

protected:
    void onCreate(lv_obj_t *root) override;

private:
    static void gestureCb(lv_event_t *e);
    UiManager &ui_;
};
