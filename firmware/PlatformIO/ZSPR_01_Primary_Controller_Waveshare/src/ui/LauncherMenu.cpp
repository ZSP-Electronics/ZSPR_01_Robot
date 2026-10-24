#include "LauncherMenu.h"
#include "UiManager.h"

#include <math.h>

namespace
{
// Angles are screen angles: 0 deg = right (+x), 90 deg = down (+y).
constexpr int kIconSize = 84;
constexpr int kEdgeMargin = 12;
constexpr int kArcStartDeg = 20;
constexpr int kArcEndDeg = 160;
} // namespace

void LauncherMenu::onCreate(lv_obj_t *root)
{
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_80, 0);
    lv_obj_add_event_cb(root, gestureCb, LV_EVENT_GESTURE, this);
    lv_obj_add_event_cb(root, backgroundTapCb, LV_EVENT_CLICKED, this);

    const int w = lv_obj_get_width(root);
    const int h = lv_obj_get_height(root);
    const int radius = w / 2 - kIconSize / 2 - kEdgeMargin;
    const auto &apps = ui_.apps();
    const int count = apps.size();

    for (int i = 0; i < count; i++)
    {
        const int deg = (count == 1) ? 90 : kArcStartDeg + i * (kArcEndDeg - kArcStartDeg) / (count - 1);
        const float rad = deg * (float)M_PI / 180.0f;

        lv_obj_t *btn = lv_button_create(root);
        lv_obj_set_size(btn, kIconSize, kIconSize);
        lv_obj_set_pos(btn, w / 2 + (int)(radius * cosf(rad)) - kIconSize / 2, h / 2 + (int)(radius * sinf(rad)) - kIconSize / 2);
        lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_set_style_pad_row(btn, 2, 0);
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_add_event_cb(btn, iconCb, LV_EVENT_CLICKED, this);
        lv_obj_set_user_data(btn, apps[i]);

        lv_obj_t *icon = lv_label_create(btn);
        lv_label_set_text(icon, apps[i]->icon());
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);

        lv_obj_t *name = lv_label_create(btn);
        lv_label_set_text(name, apps[i]->name());
        lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);
    }
}

void LauncherMenu::gestureCb(lv_event_t *e)
{
    if (lv_indev_get_gesture_dir(lv_indev_active()) == LV_DIR_BOTTOM)
        static_cast<LauncherMenu *>(lv_event_get_user_data(e))->ui_.hideLauncher();
}

void LauncherMenu::backgroundTapCb(lv_event_t *e)
{
    // Clicks on the icons bubble up here too; only react to a tap on the background itself.
    if (lv_event_get_target(e) == lv_event_get_current_target(e))
        static_cast<LauncherMenu *>(lv_event_get_user_data(e))->ui_.hideLauncher();
}

void LauncherMenu::iconCb(lv_event_t *e)
{
    auto *self = static_cast<LauncherMenu *>(lv_event_get_user_data(e));
    auto *app = static_cast<App *>(lv_obj_get_user_data((lv_obj_t *)lv_event_get_current_target(e)));
    self->ui_.openApp(*app);
}
