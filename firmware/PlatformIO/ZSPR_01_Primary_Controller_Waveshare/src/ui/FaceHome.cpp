#include "FaceHome.h"
#include "UiManager.h"

void FaceHome::onCreate(lv_obj_t *root)
{
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(root, gestureCb, LV_EVENT_GESTURE, this);
}

void FaceHome::gestureCb(lv_event_t *e)
{
    if (lv_indev_get_gesture_dir(lv_indev_active()) == LV_DIR_TOP)
        static_cast<FaceHome *>(lv_event_get_user_data(e))->ui_.showLauncher();
}
