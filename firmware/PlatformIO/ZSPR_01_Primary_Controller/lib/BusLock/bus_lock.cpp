#include "bus_lock.h"

namespace
{
    SemaphoreHandle_t g_bus[3] = {nullptr, nullptr, nullptr};
    portMUX_TYPE g_createMux = portMUX_INITIALIZER_UNLOCKED;

    // Created on first use so there is no dependence on static-init order
    // (modules and the HAL are global objects). Two tasks racing the first
    // call both create one; the loser's is deleted.
    SemaphoreHandle_t mutexFor(Bus bus)
    {
        SemaphoreHandle_t &slot = g_bus[(uint8_t)bus];
        if (!slot)
        {
            SemaphoreHandle_t fresh = xSemaphoreCreateRecursiveMutex();
            bool won;
            portENTER_CRITICAL(&g_createMux);
            won = (slot == nullptr);
            if (won)
                slot = fresh;
            portEXIT_CRITICAL(&g_createMux);
            if (!won)
                vSemaphoreDelete(fresh);
        }
        return slot;
    }
}

void bus_take(Bus bus)
{
    xSemaphoreTakeRecursive(mutexFor(bus), portMAX_DELAY);
}

void bus_give(Bus bus)
{
    xSemaphoreGiveRecursive(mutexFor(bus));
}
