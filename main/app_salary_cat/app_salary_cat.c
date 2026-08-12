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
static const char *MP3_PATH = "/sdcard/music.mp3";

static const uint32_t PWRON_SHUTDOWN_PRESS_MS = 3000U;
static const uint32_t PRESS_SCAN_PERIOD_MS = 20U;
static const uint32_t TASK_PRESS_STACK_SIZE = 4096U;
static const UBaseType_t TASK_PRESS_PRIORITY = 3U;

static const uint32_t TASK_AUDIO_STACK_SIZE = 8192U;
static const UBaseType_t TASK_AUDIO_PRIORITY = 5U;
static const uint32_t AUDIO_REOPEN_DELAY_MS = 1000U;

static TaskHandle_t s_task_press_handle;
static TaskHandle_t s_task_audio_handle;

static bool display_lock_forever(void)
{
#if defined(SALARY_CAT_BOARD_AMOLED_175)
    return bsp_display_lock(UINT32_MAX) == ESP_OK;
#else
    return bsp_display_lock(UINT32_MAX);
#endif
}

static int adjust_volume(int delta, void *user_ctx)
{
    (void)user_ctx;
    audio_player_adjust_volume(delta);
    return audio_player_get_volume();
}

static void stop_tasks(void)
{
    if (s_task_audio_handle != NULL) {
        vTaskDelete(s_task_audio_handle);
        s_task_audio_handle = NULL;
    }
    if (s_task_press_handle != NULL) {
        vTaskDelete(s_task_press_handle);
        s_task_press_handle = NULL;
    }
}

static void rollback_hardware(void)
{
    esp_err_t ret = audio_player_deinit();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Audio rollback failed: %s", esp_err_to_name(ret));
    }
    ret = pmu_power_deinit();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PMU rollback failed: %s", esp_err_to_name(ret));
    }
    ret = bsp_sdcard_unmount();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD rollback failed: %s", esp_err_to_name(ret));
    }
}

/* Loop the MP3 forever; retry after a short delay if the file cannot be opened. */
static void task_audio(void *arg)
{
    (void)arg;

    while (true) {
        if (audio_player_play_file(MP3_PATH) != ESP_OK) {
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
    const salary_cat_model_t model = {
        .adjust_volume = adjust_volume,
        .user_ctx = NULL,
    };

    if (bsp_display_start() == NULL) {
        ESP_LOGE(TAG, "Display initialization failed");
        return;
    }

    esp_err_t ret = bsp_sdcard_mount();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD card mount failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "SD card mounted at %s", BSP_SD_MOUNT_POINT);

    ret = audio_player_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio player: %s", esp_err_to_name(ret));
        rollback_hardware();
        return;
    }

    ret = pmu_power_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PMU: %s", esp_err_to_name(ret));
        rollback_hardware();
        return;
    }

    if (!display_lock_forever()) {
        ESP_LOGE(TAG, "Failed to lock display for UI initialization");
        rollback_hardware();
        return;
    }
    salary_cat_ui_init(&model);
    bsp_display_unlock();

    ret = salary_cat_load_gif();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "GIF caching failed (%s), will stream from SD", esp_err_to_name(ret));
    }

    if (xTaskCreate(task_audio, "salary_audio", TASK_AUDIO_STACK_SIZE, NULL,
                    TASK_AUDIO_PRIORITY, &s_task_audio_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio task");
        rollback_hardware();
        return;
    }

    if (xTaskCreate(task_power_button, "salary_power", TASK_PRESS_STACK_SIZE, NULL,
                    TASK_PRESS_PRIORITY, &s_task_press_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create power-button task");
        stop_tasks();
        rollback_hardware();
        return;
    }

    if (!display_lock_forever()) {
        ESP_LOGE(TAG, "Failed to lock display for playback");
        stop_tasks();
        rollback_hardware();
        return;
    }
    salary_cat_start_playback();
    bsp_display_unlock();
}
