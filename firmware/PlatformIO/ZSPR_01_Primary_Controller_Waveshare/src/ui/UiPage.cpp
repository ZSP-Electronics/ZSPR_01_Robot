#include "UiPage.h"

void UiPage::open(lv_obj_t *parent)
{
    if (root_)
        return;

    root_ = lv_obj_create(parent);
    lv_obj_remove_style_all(root_);
    lv_obj_set_size(root_, lv_pct(100), lv_pct(100));
    lv_obj_remove_flag(root_, LV_OBJ_FLAG_SCROLLABLE);
    onCreate(root_);
}

void UiPage::close()
{
    if (!root_)
        return;
    lv_obj_t *root = root_;
    release();
    lv_obj_delete(root);
}

void UiPage::release()
{
    if (!root_)
        return;
    onDestroy();
    root_ = nullptr;
}
