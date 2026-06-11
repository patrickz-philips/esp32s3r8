#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#ifdef LVGL_IN_USED
#include "battery_monitor.h"
#include "pmu_power.h"
#endif

void app_main(void)
{

    bsp_display_start();

    bsp_display_lock(-1);

#ifdef LVGL_IN_USED
    battery_monitor_ui_init();
    ESP_ERROR_CHECK(pmu_power_monitor_start());
#endif

    bsp_display_unlock();
}