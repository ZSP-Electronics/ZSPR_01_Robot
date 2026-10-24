#pragma once

#include <lvgl.h>

// Base for everything that owns LVGL widgets: the face, menus and apps.
// A page builds its widgets into a container it creates inside the parent it is opened on.
// All methods must be called with the LVGL lock held.
class UiPage
{
public:
    virtual ~UiPage() = default;

    // Creates the full-screen root container inside `parent` and calls onCreate().
    void open(lv_obj_t *parent);

    // Deletes the widgets.
    void close();

    // The widgets are about to be deleted by someone else (e.g. their screen is auto-deleted
    // after an animation). Calls onDestroy() and forgets them without deleting.
    void release();

    bool isOpen() const { return root_ != nullptr; }
    lv_obj_t *root() const { return root_; }

protected:
    virtual void onCreate(lv_obj_t *root) = 0;
    virtual void onDestroy() {} // clear cached widget pointers here

    lv_obj_t *root_ = nullptr;
};

// A launchable application. Name and icon appear in the launcher.
class App : public UiPage
{
public:
    virtual const char *name() const = 0;
    virtual const char *icon() const = 0; // an LV_SYMBOL_* string
};

// A menu or overlay (launcher, quick settings, ...).
class Menu : public UiPage
{
};
