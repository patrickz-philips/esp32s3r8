#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_memory_utils.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/spi_master.h"
#include "driver/sdspi_host.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "esp_lv_decoder.h"
#include "slide_player.h"
#include "board_config.h"

static const char *TAG = "main";
static sdmmc_card_t *s_sd_card;
static esp_lv_decoder_handle_t s_decoder_handle;

static void slide_player_unmount_sdcard(void)
{
    if (s_sd_card != NULL) {
        esp_err_t unmount_ret = esp_vfs_fat_sdcard_unmount(BOARD_SD_MOUNT_POINT, s_sd_card);
        if (unmount_ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to unmount SD card: %s", esp_err_to_name(unmount_ret));
        }
        s_sd_card = NULL;
    }

    esp_err_t bus_free_ret = spi_bus_free(SPI3_HOST);
    if (bus_free_ret != ESP_OK && bus_free_ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "Failed to free SD SPI bus: %s", esp_err_to_name(bus_free_ret));
    }
}

static esp_err_t slide_player_mount_sdcard(void)
{
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST;

    const spi_bus_config_t bus_cfg = {
        .mosi_io_num = BOARD_SD_PIN_MOSI,
        .miso_io_num = BOARD_SD_PIN_MISO,
        .sclk_io_num = BOARD_SD_PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SD SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    const esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 8,
        .allocation_unit_size = 16 * 1024,
    };

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = BOARD_SD_PIN_CS;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(
        BOARD_SD_MOUNT_POINT,
        &host,
        &slot_config,
        &mount_config,
        &s_sd_card
    );
    if (ret != ESP_OK) {
        spi_bus_free(host.slot);
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SD card mounted at %s", BOARD_SD_MOUNT_POINT);
    sdmmc_card_print_info(stdout, s_sd_card);
    return ESP_OK;
}

static esp_err_t slide_player_runtime_init(void)
{
    esp_err_t ret = slide_player_mount_sdcard();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD card mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

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
