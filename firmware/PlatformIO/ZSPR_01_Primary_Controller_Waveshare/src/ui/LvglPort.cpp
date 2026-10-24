#include "LvglPort.h"
#include "Touch_GT911.h"

namespace
{
constexpr int kTaskCore = 1; // core 0 stays free for the robot tasks
constexpr int kTaskPrio = 3;
constexpr int kTaskStack = 8192;
constexpr int kBufLines = 40; // partial render buffers: two stripes of 40 lines
constexpr uint32_t kReleaseTimeoutMs = 60; // GT911 reports ~every 10 ms while a finger is down

RGBPanel *s_panel;
SemaphoreHandle_t s_mutex;
bool s_direct; // true: LVGL renders into the driver's two frame buffers and we swap on vsync
} // namespace

bool LvglPort::begin(RGBPanel &panel)
{
    s_panel = &panel;
    s_mutex = xSemaphoreCreateRecursiveMutex();
    if (!s_mutex)
        return false;

    const uint32_t w = panel.width();
    const uint32_t h = panel.height();

    lv_init();
    lv_tick_set_cb([]() -> uint32_t { return millis(); });

    lv_display_t *disp = lv_display_create(w, h);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(disp, flushCb);

    s_direct = panel.frameBufferCount() == 2;
    if (s_direct)
    {
        // Tear-free: LVGL draws only the changed areas straight into the off-screen frame buffer
        // (keeping the other one in sync), then the panel swaps to it on vsync.
        lv_display_set_buffers(disp, panel.frameBuffer(0), panel.frameBuffer(1),
                               w * h * sizeof(uint16_t), LV_DISPLAY_RENDER_MODE_DIRECT);
    }
    else
    {
        // Single frame buffer: draw stripes in internal RAM (fast for the CPU); pushColors() copies them in.
        const size_t buf_bytes = w * kBufLines * sizeof(uint16_t);
        void *buf1 = heap_caps_malloc(buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        void *buf2 = heap_caps_malloc(buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!buf1 || !buf2)
            return false;
        lv_display_set_buffers(disp, buf1, buf2, buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    }

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchCb);
    lv_indev_set_display(indev, disp);

    return xTaskCreatePinnedToCore(task, "lvgl", kTaskStack, nullptr, kTaskPrio, nullptr, kTaskCore) == pdPASS;
}

bool LvglPort::lock(uint32_t timeout_ms)
{
    const TickType_t ticks = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(s_mutex, ticks) == pdTRUE;
}

void LvglPort::unlock()
{
    xSemaphoreGiveRecursive(s_mutex);
}

void LvglPort::flushCb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    if (s_direct)
    {
        // px_map is the start of the whole frame buffer; only swap once the last dirty area is drawn.
        if (lv_display_flush_is_last(disp))
            s_panel->showFrameBuffer((uint16_t *)px_map);
    }
    else
    {
        s_panel->pushColors(area->x1, area->y1, lv_area_get_width(area), lv_area_get_height(area), (uint16_t *)px_map);
    }
    lv_display_flush_ready(disp);
}

void LvglPort::touchCb(lv_indev_t *, lv_indev_data_t *data)
{
    static uint16_t last_x, last_y;
    static uint32_t last_seen_ms;
    static bool pressed;

    // Touch_Get_XY() consumes the point, so a poll between two controller frames reads as "no touch".
    // Only report a release once the controller has been quiet for kReleaseTimeoutMs.
    Touch_Read_Data();
    uint16_t x[1], y[1];
    uint8_t count = 0;
    if (Touch_Get_XY(x, y, nullptr, &count, 1) && count > 0)
    {
        last_x = x[0];
        last_y = y[0];
        last_seen_ms = millis();
        pressed = true;
    }
    else if (pressed && millis() - last_seen_ms > kReleaseTimeoutMs)
    {
        pressed = false;
    }

    data->point.x = last_x;
    data->point.y = last_y;
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void LvglPort::task(void *)
{
    for (;;)
    {
        lock();
        uint32_t wait_ms = lv_timer_handler();
        unlock();
        vTaskDelay(pdMS_TO_TICKS(constrain(wait_ms, 1u, 16u)));
    }
}
