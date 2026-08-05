#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#ifdef LVGL_IN_USED
#include "acc_data.h"
#include "model.h"
#include "sd_acc_writer.h"
#include "task.h"
#endif

static const BaseType_t LVGL_TASK_CORE = 0;

void app_main(void)
{
    bsp_display_cfg_t display_config = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,
        },
    };
    display_config.lvgl_port_cfg.task_affinity = LVGL_TASK_CORE;

    bsp_display_start_with_config(&display_config);

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