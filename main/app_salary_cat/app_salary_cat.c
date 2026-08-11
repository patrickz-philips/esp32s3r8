/**
 * @file app_salary_cat.c
 * @brief Entry point for the "salary_cat" app: boots straight into a looping
 *        GIF + MP3 played from the SD card. A 3 s long-press on the PWR key
 *        (AXP2101 PEKEY) powers the board off.
 */

#include "bsp/esp-bsp.h"
#include "bsp/display.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "audio_player.h"
#include "pmu_power.h"
#include "salary_cat.h"

static const char *TAG = "app_salary_cat";

static const uint32_t PWRON_SHUTDOWN_PRESS_MS = 3000U;
static const uint32_t PRESS_SCAN_PERIOD_MS = 20U;
static const uint32_t TASK_PRESS_STACK_SIZE = 4096U;
static const UBaseType_t TASK_PRESS_PRIORITY = 3U;

static const uint32_t TASK_AUDIO_STACK_SIZE = 8192U;
static const UBaseType_t TASK_AUDIO_PRIORITY = 5U;
static const uint32_t AUDIO_REOPEN_DELAY_MS = 1000U;

static TaskHandle_t s_task_press_handle;
static TaskHandle_t s_task_audio_handle;

/* Loop the MP3 forever; retry after a short delay if the file cannot be opened. */
static void task_audio(void *arg)
{
    (void)arg;

    while (true) {
        if (audio_player_play_file(AUDIO_PLAYER_MP3_PATH) != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(AUDIO_REOPEN_DELAY_MS));
        }
    }
}

/* Poll the AXP2101 power key and shut down after a 3 s continuous long-press. */
static void task_power_button(void *arg)
{
    (void)arg;

    bool pressed = false;
    TickType_t press_start_tick = 0;
    TickType_t last_wake_tick = xTaskGetTickCount();

    while (true) {
        bool pressed_edge = false;
        bool released_edge = false;

        if (pmu_power_poll_button(&pressed_edge, &released_edge) == ESP_OK) {
            const TickType_t now = xTaskGetTickCount();

            if (pressed_edge) {
                pressed = true;
                press_start_tick = now;
            }
            if (released_edge) {
                pressed = false;
            }

            if (pressed && (now - press_start_tick) >= pdMS_TO_TICKS(PWRON_SHUTDOWN_PRESS_MS)) {
                ESP_LOGI(TAG, "PWR long-press >= %u ms, shutting down", (unsigned int)PWRON_SHUTDOWN_PRESS_MS);
                esp_err_t ret = pmu_power_shutdown();
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to shut down PMU: %s", esp_err_to_name(ret));
                }
                pressed = false;
            }
        }

        vTaskDelayUntil(&last_wake_tick, pdMS_TO_TICKS(PRESS_SCAN_PERIOD_MS));
    }
}

void app_main(void)
{
    bsp_display_start();

    bsp_display_lock(-1);
    salary_cat_ui_init();
    bsp_display_unlock();

    esp_err_t ret = bsp_sdcard_mount();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD card mount failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "SD card mounted at %s", BSP_SD_MOUNT_POINT);

    /* Cache the GIF into RAM first (streaming from the SD card is too slow). */
    ret = salary_cat_load_gif();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "GIF caching failed (%s), will stream from SD", esp_err_to_name(ret));
    }

    /* Start the GIF (from the cache) and the MP3 together. */
    bsp_display_lock(-1);
    salary_cat_start_playback();
    bsp_display_unlock();

    ret = audio_player_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio player: %s", esp_err_to_name(ret));
    } else if (xTaskCreate(task_audio, "audio_player", TASK_AUDIO_STACK_SIZE, NULL,
                           TASK_AUDIO_PRIORITY, &s_task_audio_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio task");
    }

    ret = pmu_power_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PMU: %s", esp_err_to_name(ret));
        return;
    }

    if (xTaskCreate(task_power_button, "taskPress", TASK_PRESS_STACK_SIZE, NULL,
                    TASK_PRESS_PRIORITY, &s_task_press_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create power-button task");
    }
}
