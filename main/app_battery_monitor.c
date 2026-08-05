#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "battery_monitor.h"
#include "model.h"
#include "pmu_power.h"

static const char *TAG = "app_bm";
static const BaseType_t LVGL_TASK_CORE = 0;

static void pmu_poll_task(void *arg)
{
    (void)arg;
    const TickType_t period = pdMS_TO_TICKS(200);
    for (;;) {
        pmu_power_data_t pd = {0};
        if (pmu_power_read_data(&pd) == ESP_OK) {
            battery_monitor_data_t bd = {
                .temperature_x10 = pd.temperature_x10,
                .bat_voltage_mv = pd.bat_voltage_mv,
                .vbus_voltage_mv = pd.vbus_voltage_mv,
                .system_voltage_mv = pd.system_voltage_mv,
                .bat_percent = pd.bat_percent,
                .is_charging = pd.is_charging,
                .is_discharge = pd.is_discharge,
                .is_standby = pd.is_standby,
                .is_vbus_in = pd.is_vbus_in,
                .is_vbus_good = pd.is_vbus_good,
                .charge_status = pd.charge_status,
            };
            model_post_pmu_data(&bd);
        }

        bool pressed_edge = false;
        bool released_edge = false;
        if (pmu_power_poll_button(&pressed_edge, &released_edge) == ESP_OK && released_edge) {
            model_post_button_event(BATTERY_MONITOR_BUTTON_SOURCE_PWRON,
                                    BATTERY_MONITOR_BUTTON_PRESS_SHORT);
        }

        vTaskDelay(period);
    }
}

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

    ESP_ERROR_CHECK(pmu_power_init());

    bsp_display_lock(-1);
    battery_monitor_ui_init();
    ESP_ERROR_CHECK(model_init());
    bsp_display_unlock();

    if (xTaskCreatePinnedToCore(pmu_poll_task, "pmu_poll", 4096, NULL, 4, NULL, 1) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create PMU poll task");
    }
}
