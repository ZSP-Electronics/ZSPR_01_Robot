#include "UiManager.h"

void UiManager::begin(UiPage &home, Menu &launcher)
{
    home_ = &home;
    launcher_ = &launcher;

    home_screen_ = lv_screen_active();
    lv_obj_set_style_bg_color(home_screen_, lv_color_black(), 0);
    home.open(home_screen_);

    launcher.open(lv_layer_top());
    lv_obj_add_flag(launcher.root(), LV_OBJ_FLAG_HIDDEN);
}

void UiManager::showLauncher()
{
    if (running_ || !launcher_ || launcher_visible_)
        return;
    lv_obj_remove_flag(launcher_->root(), LV_OBJ_FLAG_HIDDEN);
    launcher_visible_ = true;
}

void UiManager::hideLauncher()
{
    if (!launcher_ || !launcher_visible_)
        return;
    lv_obj_add_flag(launcher_->root(), LV_OBJ_FLAG_HIDDEN);
    launcher_visible_ = false;
}

void UiManager::openApp(App &app)
{
    if (running_)
        return;
    hideLauncher();

    // Each app gets its own screen, deleted again when the app closes.
    app_screen_ = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(app_screen_, lv_color_black(), 0);
    app.open(app_screen_);

    // The manager adds the back button on top of the app's content, top left.
    lv_obj_t *back = lv_button_create(app_screen_);
    lv_obj_set_size(back, 56, 56);
    lv_obj_align(back, LV_ALIGN_TOP_LEFT, 8, 8);
    lv_obj_add_event_cb(back, backCb, LV_EVENT_CLICKED, this);
    lv_obj_t *arrow = lv_label_create(back);
    lv_label_set_text(arrow, LV_SYMBOL_LEFT);
    lv_obj_center(arrow);

    running_ = &app;
    lv_screen_load_anim(app_screen_, LV_SCR_LOAD_ANIM_MOVE_TOP, 250, 0, false);
}

void UiManager::closeApp()
{
    if (!running_)
        return;
    running_->release(); // its widgets go away with the screen below
    running_ = nullptr;
    app_screen_ = nullptr;
    lv_screen_load_anim(home_screen_, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0, true); // auto-deletes the app screen
}

void UiManager::backCb(lv_event_t *e)
{
    static_cast<UiManager *>(lv_event_get_user_data(e))->closeApp();
}
