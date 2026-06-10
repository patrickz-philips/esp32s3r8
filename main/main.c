#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#ifdef LVGL_IN_USED
#include "battery_monitor.h"
#endif

void app_main(void)
{

    bsp_display_start();

    bsp_display_lock(-1);

#ifdef LVGL_IN_USED
    battery_monitor_ui_init();
#endif

    bsp_display_unlock();
}