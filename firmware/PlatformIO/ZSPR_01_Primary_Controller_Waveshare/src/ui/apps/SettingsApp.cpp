#include "SettingsApp.h"
#include "RGBPanel.h"
#include "TCA9554PWR.h"

void SettingsApp::onCreate(lv_obj_t *root)
{
    // Below the manager's back button; flex column scrolls vertically when it overflows.
    list_ = lv_obj_create(root);
    lv_obj_set_size(list_, 400, 380);
    lv_obj_align(list_, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_flex_flow(list_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list_, 24, 0);

    lv_label_set_text(lv_label_create(list_), "Backlight");
    lv_obj_t *slider = lv_slider_create(list_);
    lv_obj_set_width(slider, lv_pct(90));
    lv_slider_set_range(slider, 5, 100);
    lv_slider_set_value(slider, LCD_Backlight, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, backlightCb, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_label_set_text(lv_label_create(list_), "Buzzer");
    lv_obj_t *sw = lv_switch_create(list_);
    lv_obj_add_event_cb(sw, buzzerCb, LV_EVENT_VALUE_CHANGED, nullptr);
}

void SettingsApp::backlightCb(lv_event_t *e)
{
    LCD_Backlight = lv_slider_get_value((lv_obj_t *)lv_event_get_target(e));
    Set_Backlight(LCD_Backlight);
}

void SettingsApp::buzzerCb(lv_event_t *e)
{
    const bool on = lv_obj_has_state((lv_obj_t *)lv_event_get_target(e), LV_STATE_CHECKED);
    Set_EXIO(EXIO_PIN8, on ? High : Low);
}
