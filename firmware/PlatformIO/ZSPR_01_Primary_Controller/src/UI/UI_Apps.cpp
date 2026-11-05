#include "UI_Apps.h"

uint8_t ui_register_apps(HAL_Robot_Module &robot, UI_Home &home)
{
    uint8_t n = 0;
#define UI_APP(Class)                                  \
    {                                                  \
        static Class app(robot);                       \
        if (home.addApp(&app))                     \
            n++;                                       \
    }
    UI_APP_LIST(UI_APP)
#undef UI_APP
    return n;
}
