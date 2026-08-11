#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "esp_lv_decoder.h"
#include "slide_player.h"

static const char *TAG = "main";
static esp_lv_decoder_handle_t s_decoder_handle;

// SD wiring differs per board (206: SDMMC, 175: SPI); the board BSP owns it.
static void slide_player_unmount_sdcard(void)
{
    esp_err_t unmount_ret = bsp_sdcard_unmount();
    if (unmount_ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to unmount SD card: %s", esp_err_to_name(unmount_ret));
    }
}

static esp_err_t slide_player_runtime_init(void)
{
    esp_err_t ret = bsp_sdcard_mount();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD card mount failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "SD card mounted at %s", BSP_SD_MOUNT_POINT);

    ret = esp_lv_decoder_init(&s_decoder_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LVGL decoder init failed: %s", esp_err_to_name(ret));
        slide_player_unmount_sdcard();
        return ret;
    }

    return ESP_OK;
}

void app_main(void)
{

    bsp_display_start();

    bsp_display_lock(-1);

    esp_err_t ret = slide_player_runtime_init();
    if (ret != ESP_OK) {
        bsp_display_unlock();
        return;
    }

    slide_player_ui_init();

    bsp_display_unlock();
}
