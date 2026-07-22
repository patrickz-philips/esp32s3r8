#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#ifdef LVGL_IN_USED
#include "acc_data.h"
#include "model.h"
#include "sd_acc_writer.h"
#include "task.h"
#endif

void app_main(void)
{

    bsp_display_start();

    bsp_display_lock(-1);

#ifdef LVGL_IN_USED
    acc_data_ui_init();
    ESP_ERROR_CHECK(model_init());
#endif

    bsp_display_unlock();

#ifdef LVGL_IN_USED
    ESP_ERROR_CHECK(sd_acc_writer_init());
    ESP_ERROR_CHECK(app_tasks_start());
#endif
}