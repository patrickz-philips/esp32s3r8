#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#ifdef LVGL_IN_USED
#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "acc_data.h"
#include "imu.h"
#include "model.h"
#include "pmu_power.h"
#include "sd_acc_writer.h"
#endif

#ifdef LVGL_IN_USED
static const char * TAG = "app_task";

static const uint32_t PRESS_SCAN_PERIOD_MS = 10U;
static const uint32_t BUTTON_DEBOUNCE_MS = 30U;
static const uint32_t BUTTON_LONG_PRESS_MS = 1500U;
static const uint32_t PWRON_SHUTDOWN_PRESS_MS = 3000U;
static const uint32_t HAPTIC_PULSE_MS = 40U;
static const uint32_t IMU_POLL_PERIOD_MS = 20U;
static const uint32_t SD_DROP_LOG_PERIOD_MS = 1000U;

static const UBaseType_t TASK_PMU_PRIORITY = 1U;
static const UBaseType_t TASK_IMU_PRIORITY = 2U;
static const UBaseType_t TASK_PRESS_PRIORITY = 2U;
static const UBaseType_t TASK_HAPTIC_PRIORITY = 3U;
static const UBaseType_t TASK_LVGL_PRIORITY = 4U;

static const uint32_t TASK_PMU_STACK_SIZE = 4096U;
static const uint32_t TASK_IMU_STACK_SIZE = 4096U;
static const uint32_t TASK_PRESS_STACK_SIZE = 4096U;
static const uint32_t TASK_HAPTIC_STACK_SIZE = 2048U;

static const gpio_num_t BOOT_BUTTON_GPIO = GPIO_NUM_0;
static const gpio_num_t HAPTIC_GPIO = GPIO_NUM_18;

typedef struct {
    bool stable_pressed;
    bool sampled_pressed;
    TickType_t last_edge_tick;
    TickType_t press_start_tick;
} debounced_button_t;

static TaskHandle_t s_task_pmu_handle;
static TaskHandle_t s_task_imu_handle;
static TaskHandle_t s_task_press_handle;
static TaskHandle_t s_task_haptic_handle;

static void notify_haptic_task(void)
{
    if (s_task_haptic_handle != NULL) {
        xTaskNotifyGive(s_task_haptic_handle);
    }
}

static esp_err_t button_gpio_init(void)
{
    gpio_config_t config = {0};
    config.pin_bit_mask = 1ULL << BOOT_BUTTON_GPIO;
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    return gpio_config(&config);
}

static esp_err_t haptic_gpio_init(void)
{
    gpio_config_t config = {0};
    config.pin_bit_mask = 1ULL << HAPTIC_GPIO;
    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    ESP_RETURN_ON_ERROR(gpio_config(&config), TAG, "Failed to configure haptic GPIO");
    return gpio_set_level(HAPTIC_GPIO, 0);
}

static void process_boot_button(debounced_button_t * state)
{
    const TickType_t now = xTaskGetTickCount();
    const bool raw_pressed = gpio_get_level(BOOT_BUTTON_GPIO) == 0;

    if (raw_pressed != state->sampled_pressed) {
        state->sampled_pressed = raw_pressed;
        state->last_edge_tick = now;
    }

    if ((now - state->last_edge_tick) < pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS) || raw_pressed == state->stable_pressed) {
        return;
    }

    state->stable_pressed = raw_pressed;
    if (state->stable_pressed) {
        state->press_start_tick = now;
        notify_haptic_task();
        return;
    }

    const TickType_t held_ticks = now - state->press_start_tick;
    model_post_button_event(MODEL_BUTTON_SOURCE_GPIO0,
                            held_ticks >= pdMS_TO_TICKS(BUTTON_LONG_PRESS_MS) ? MODEL_BUTTON_PRESS_LONG
                                                                             : MODEL_BUTTON_PRESS_SHORT);
}

static void process_pwron_button(debounced_button_t * state)
{
    bool pressed_edge = false;
    bool release_edge = false;
    if (pmu_power_poll_button(&pressed_edge, &release_edge) != ESP_OK) {
        return;
    }

    const TickType_t now = xTaskGetTickCount();

    if (pressed_edge) {
        state->stable_pressed = true;
        state->press_start_tick = now;
        notify_haptic_task();
    }

    if (release_edge && state->stable_pressed) {
        state->stable_pressed = false;
        const TickType_t held_ticks = now - state->press_start_tick;
        if (held_ticks >= pdMS_TO_TICKS(PWRON_SHUTDOWN_PRESS_MS)) {
            esp_err_t ret = pmu_power_shutdown();
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to shut down PMU: %s", esp_err_to_name(ret));
            }
            return;
        }

        model_post_button_event(MODEL_BUTTON_SOURCE_PWRON,
                                held_ticks >= pdMS_TO_TICKS(BUTTON_LONG_PRESS_MS) ? MODEL_BUTTON_PRESS_LONG
                                                                                 : MODEL_BUTTON_PRESS_SHORT);
    }
}

static void task_pmu(void * arg)
{
    (void)arg;

    TickType_t last_wake_tick = xTaskGetTickCount();
    while (true) {
        pmu_power_data_t data = {0};
        if (pmu_power_read_data(&data) == ESP_OK) {
            model_post_pmu_data(&data);
        } else {
            ESP_LOGW(TAG, "taskPMU skipped one sample because PMU read failed");
        }

        vTaskDelayUntil(&last_wake_tick, pdMS_TO_TICKS(1000U));
    }
}

static void task_imu(void * arg)
{
    imu_acc_data_t data = {0};
    bool first_sample_logged = false;
    uint32_t dropped_sd_samples = 0U;
    TickType_t last_sd_drop_log_tick = xTaskGetTickCount() - pdMS_TO_TICKS(SD_DROP_LOG_PERIOD_MS);
    (void)arg;

    TickType_t last_wake_tick = xTaskGetTickCount();
    while (true) {
        esp_err_t ret = imu_read_acc(&data);
        if (ret == ESP_OK) {
            const acc_data_sample_t sample = {
                .x = data.x,
                .y = data.y,
                .z = data.z,
            };
            if (!model_post_acc_data(&sample)) {
                ESP_LOGW(TAG, "taskIMU failed to post sample to model");
            } else if (!first_sample_logged) {
                ESP_LOGI(TAG, "taskIMU first sample: x=%d, y=%d, z=%d",
                         (int)sample.x, (int)sample.y, (int)sample.z);
                first_sample_logged = true;
            }
            if (!sd_acc_writer_post_sample(&sample)) {
                dropped_sd_samples++;
                const TickType_t now = xTaskGetTickCount();
                if ((now - last_sd_drop_log_tick) >= pdMS_TO_TICKS(SD_DROP_LOG_PERIOD_MS)) {
                    ESP_LOGW(TAG, "SD writer queue full, dropped %lu samples",
                             (unsigned long)dropped_sd_samples);
                    dropped_sd_samples = 0U;
                    last_sd_drop_log_tick = now;
                }
            }
        } else {
            ESP_LOGW(TAG, "taskIMU skipped one sample: %s", esp_err_to_name(ret));
        }

        vTaskDelayUntil(&last_wake_tick, pdMS_TO_TICKS(IMU_POLL_PERIOD_MS));
    }
}

static void task_press(void * arg)
{
    (void)arg;

    debounced_button_t boot_button = {0};
    debounced_button_t pwron_button = {0};

    boot_button.sampled_pressed = gpio_get_level(BOOT_BUTTON_GPIO) == 0;
    boot_button.stable_pressed = boot_button.sampled_pressed;
    boot_button.last_edge_tick = xTaskGetTickCount();
    boot_button.press_start_tick = boot_button.stable_pressed ? boot_button.last_edge_tick : 0;

    TickType_t last_wake_tick = xTaskGetTickCount();
    while (true) {
        process_boot_button(&boot_button);
        process_pwron_button(&pwron_button);
        vTaskDelayUntil(&last_wake_tick, pdMS_TO_TICKS(PRESS_SCAN_PERIOD_MS));
    }
}

static void task_haptic(void * arg)
{
    (void)arg;

    while (true) {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        gpio_set_level(HAPTIC_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(HAPTIC_PULSE_MS));
        gpio_set_level(HAPTIC_GPIO, 0);
    }
}

static esp_err_t acc_data_tasks_start(void)
{
    BaseType_t ret;

    if (s_task_pmu_handle != NULL && s_task_imu_handle != NULL &&
        s_task_press_handle != NULL && s_task_haptic_handle != NULL) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(pmu_power_init(), TAG, "Failed to initialize PMU");
    ESP_RETURN_ON_ERROR(imu_init(), TAG, "Failed to initialize IMU");
    ESP_RETURN_ON_ERROR(button_gpio_init(), TAG, "Failed to configure GPIO0 button");
    ESP_RETURN_ON_ERROR(haptic_gpio_init(), TAG, "Failed to configure haptic GPIO");

    ret = xTaskCreate(task_haptic, "taskHaptic", TASK_HAPTIC_STACK_SIZE, NULL,
                      TASK_HAPTIC_PRIORITY, &s_task_haptic_handle);
    ESP_RETURN_ON_FALSE(ret == pdPASS, ESP_ERR_NO_MEM, TAG, "Failed to create taskHaptic");

    ret = xTaskCreate(task_press, "taskPress", TASK_PRESS_STACK_SIZE, NULL,
                      TASK_PRESS_PRIORITY, &s_task_press_handle);
    ESP_RETURN_ON_FALSE(ret == pdPASS, ESP_ERR_NO_MEM, TAG, "Failed to create taskPress");

    ret = xTaskCreate(task_pmu, "taskPMU", TASK_PMU_STACK_SIZE, NULL,
                      TASK_PMU_PRIORITY, &s_task_pmu_handle);
    ESP_RETURN_ON_FALSE(ret == pdPASS, ESP_ERR_NO_MEM, TAG, "Failed to create taskPMU");

    ret = xTaskCreate(task_imu, "taskIMU", TASK_IMU_STACK_SIZE, NULL,
                      TASK_IMU_PRIORITY, &s_task_imu_handle);
    ESP_RETURN_ON_FALSE(ret == pdPASS, ESP_ERR_NO_MEM, TAG, "Failed to create taskIMU");

    ESP_LOGI(TAG,
             "Task priorities: taskLVGL=%u, taskHaptic=%u, taskPress=%u, taskIMU=%u, taskPMU=%u",
             (unsigned int)TASK_LVGL_PRIORITY,
             (unsigned int)TASK_HAPTIC_PRIORITY,
             (unsigned int)TASK_PRESS_PRIORITY,
             (unsigned int)TASK_IMU_PRIORITY,
             (unsigned int)TASK_PMU_PRIORITY);

    return ESP_OK;
}
#endif /* LVGL_IN_USED */

void app_main(void)
{
    if (bsp_display_start() == NULL) {
        ESP_LOGE(TAG, "Display initialization failed");
        return;
    }

    bsp_display_lock(-1);

#ifdef LVGL_IN_USED
    acc_data_ui_init();
    ESP_ERROR_CHECK(model_init());
#endif

    bsp_display_unlock();

#ifdef LVGL_IN_USED
    ESP_ERROR_CHECK(sd_acc_writer_init());
    ESP_ERROR_CHECK(acc_data_tasks_start());
#endif
}