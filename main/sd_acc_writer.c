#include "sd_acc_writer.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include "bsp/esp-bsp.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char * TAG = "sd_acc_writer";
static const UBaseType_t WRITER_COMMAND_QUEUE_LENGTH = 4U;
static const UBaseType_t WRITER_SAMPLE_QUEUE_LENGTH = 512U;
static const UBaseType_t WRITER_TASK_PRIORITY = 1U;
static const uint32_t WRITER_TASK_STACK_SIZE = 4096U;
static const uint32_t SD_MOUNT_RETRY_PERIOD_MS = 10000U;

typedef enum {
    WRITER_COMMAND_START = 0,
    WRITER_COMMAND_STOP,
} writer_command_t;

static QueueHandle_t s_command_queue;
static QueueHandle_t s_sample_queue;
static TaskHandle_t s_writer_task_handle;
static uint32_t s_next_file_index;
static volatile bool s_accepting_samples;

static uint32_t find_next_file_index(void);

static bool try_mount_sdcard(void)
{
    const esp_err_t ret = bsp_sdcard_mount();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "SD card not available, retrying in %lu seconds: %s",
                 (unsigned long)(SD_MOUNT_RETRY_PERIOD_MS / 1000U), esp_err_to_name(ret));
        return false;
    }

    s_next_file_index = find_next_file_index();
    ESP_LOGI(TAG, "SD card mounted at %s, next file index=%lu",
             BSP_SD_MOUNT_POINT, (unsigned long)s_next_file_index);
    return true;
}

static void unmount_sdcard(void)
{
    const esp_err_t ret = bsp_sdcard_unmount();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
    }
    bsp_sdcard = NULL;
}

static uint32_t find_next_file_index(void)
{
    uint32_t max_index = 0U;
    DIR * directory = opendir(BSP_SD_MOUNT_POINT);
    if (directory == NULL) {
        ESP_LOGW(TAG, "Failed to scan %s", BSP_SD_MOUNT_POINT);
        return 1U;
    }

    struct dirent * entry;
    while ((entry = readdir(directory)) != NULL) {
        unsigned long index;
        char trailing;
        if (sscanf(entry->d_name, "acc_%lu.csv%c", &index, &trailing) == 1 && index > max_index) {
            max_index = (uint32_t)index;
        }
    }

    closedir(directory);
    return max_index + 1U;
}

static FILE * open_next_file(char * path, size_t path_size)
{
    snprintf(path, path_size, BSP_SD_MOUNT_POINT "/acc_%lu.csv", (unsigned long)s_next_file_index);
    FILE * file = fopen(path, "w");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open %s", path);
        return NULL;
    }

    s_next_file_index++;
    if (fprintf(file, "x,y,z\n") < 0) {
        ESP_LOGE(TAG, "Failed to write CSV header to %s", path);
        fclose(file);
        return NULL;
    }

    return file;
}

static void close_recording(FILE ** file, const char * path, uint32_t sample_count, bool success)
{
    if (*file != NULL) {
        if (fclose(*file) != 0) {
            ESP_LOGE(TAG, "Failed to close %s", path);
            success = false;
        }
        *file = NULL;
    }

    ESP_LOGI(TAG, "Recording finished: file=%s, samples=%lu, success=%d",
             path, (unsigned long)sample_count, success);
    s_accepting_samples = false;
    xQueueReset(s_sample_queue);
    model_post_sd_save_finished(success);
}

static void task_sd_acc_writer(void * arg)
{
    writer_command_t command;
    acc_data_sample_t sample;
    FILE * file = NULL;
    char path[64] = {0};
    uint32_t sample_count = 0U;
    bool stop_requested = false;
    bool sdcard_mounted = false;
    TickType_t last_mount_attempt = xTaskGetTickCount() - pdMS_TO_TICKS(SD_MOUNT_RETRY_PERIOD_MS);
    (void)arg;

    while (true) {
        const TickType_t now = xTaskGetTickCount();
        if (!sdcard_mounted &&
            (now - last_mount_attempt) >= pdMS_TO_TICKS(SD_MOUNT_RETRY_PERIOD_MS)) {
            last_mount_attempt = now;
            sdcard_mounted = try_mount_sdcard();
        }

        const bool command_received = xQueueReceive(s_command_queue, &command, 0) == pdTRUE;
        if (command_received && command == WRITER_COMMAND_START) {
            if (file != NULL) {
                ESP_LOGW(TAG, "Ignoring start request while recording");
                continue;
            }
            if (!sdcard_mounted) {
                ESP_LOGW(TAG, "Cannot start recording because no SD card is mounted");
                model_post_sd_save_finished(false);
                continue;
            }
            file = open_next_file(path, sizeof(path));
            sample_count = 0U;
            stop_requested = false;
            if (file == NULL) {
                model_post_sd_save_finished(false);
                unmount_sdcard();
                sdcard_mounted = false;
                last_mount_attempt = xTaskGetTickCount();
            } else {
                s_accepting_samples = true;
                ESP_LOGI(TAG, "Recording started: %s", path);
            }
        } else if (command_received && command == WRITER_COMMAND_STOP) {
            if (file == NULL) {
                continue;
            }
            stop_requested = true;
            if (sample_count == SD_ACC_SAMPLE_COUNT) {
                close_recording(&file, path, sample_count, true);
            }
        }

        if (xQueueReceive(s_sample_queue, &sample, pdMS_TO_TICKS(20U)) == pdTRUE &&
            file != NULL && sample_count < SD_ACC_SAMPLE_COUNT) {
            if (fprintf(file, "%d,%d,%d\n", (int)sample.x, (int)sample.y, (int)sample.z) < 0) {
                ESP_LOGE(TAG, "Failed to write sample to %s", path);
                close_recording(&file, path, sample_count, false);
                unmount_sdcard();
                sdcard_mounted = false;
                last_mount_attempt = xTaskGetTickCount();
                continue;
            }

            sample_count++;
            if (sample_count == SD_ACC_SAMPLE_COUNT) {
                s_accepting_samples = false;
                xQueueReset(s_sample_queue);
                if (stop_requested) {
                    close_recording(&file, path, sample_count, true);
                }
            }
        }
    }
}

esp_err_t sd_acc_writer_init(void)
{
    if (s_writer_task_handle != NULL) {
        return ESP_OK;
    }

    s_command_queue = xQueueCreate(WRITER_COMMAND_QUEUE_LENGTH, sizeof(writer_command_t));
    ESP_RETURN_ON_FALSE(s_command_queue != NULL, ESP_ERR_NO_MEM, TAG, "Failed to create command queue");
    s_sample_queue = xQueueCreate(WRITER_SAMPLE_QUEUE_LENGTH, sizeof(acc_data_sample_t));
    ESP_RETURN_ON_FALSE(s_sample_queue != NULL, ESP_ERR_NO_MEM, TAG, "Failed to create sample queue");

    BaseType_t ret = xTaskCreate(task_sd_acc_writer, "taskSDAcc", WRITER_TASK_STACK_SIZE, NULL,
                                 WRITER_TASK_PRIORITY, &s_writer_task_handle);
    ESP_RETURN_ON_FALSE(ret == pdPASS, ESP_ERR_NO_MEM, TAG, "Failed to create taskSDAcc");
    return ESP_OK;
}

bool sd_acc_writer_request_start(void)
{
    const writer_command_t command = WRITER_COMMAND_START;
    return s_command_queue != NULL && xQueueSendToBack(s_command_queue, &command, 0) == pdTRUE;
}

bool sd_acc_writer_request_stop(void)
{
    const writer_command_t command = WRITER_COMMAND_STOP;
    return s_command_queue != NULL && xQueueSendToBack(s_command_queue, &command, 0) == pdTRUE;
}

bool sd_acc_writer_post_sample(const acc_data_sample_t * sample)
{
    if (!s_accepting_samples) {
        return true;
    }

    return sample != NULL && s_sample_queue != NULL && xQueueSendToBack(s_sample_queue, sample, 0) == pdTRUE;
}