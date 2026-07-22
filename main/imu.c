#include "imu.h"

#include <stdbool.h>

#include "bsp/esp-bsp.h"
#include "esp_check.h"
#include "esp_log.h"
#include "qmi8658.h"

static const char * TAG = "imu";

static qmi8658_dev_t s_imu;
static bool s_initialized;

esp_err_t imu_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    i2c_master_bus_handle_t bus_handle = bsp_i2c_get_handle();
    ESP_RETURN_ON_FALSE(bus_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "BSP I2C bus is not initialized");

    ESP_RETURN_ON_ERROR(qmi8658_init(&s_imu, bus_handle, QMI8658_ADDRESS_HIGH), TAG, "Failed to initialize QMI8658");
    ESP_RETURN_ON_ERROR(qmi8658_set_accel_range(&s_imu, QMI8658_ACCEL_RANGE_4G), TAG,
                        "Failed to set accelerometer range");
    ESP_RETURN_ON_ERROR(qmi8658_set_accel_odr(&s_imu, QMI8658_ACCEL_ODR_250HZ), TAG,
                        "Failed to set accelerometer ODR");
    ESP_RETURN_ON_ERROR(qmi8658_write_register(&s_imu, QMI8658_CTRL5, 0x03), TAG,
                        "Failed to configure QMI8658 filters");
    ESP_RETURN_ON_ERROR(qmi8658_enable_gyro(&s_imu, false), TAG, "Failed to disable gyroscope");
    ESP_RETURN_ON_ERROR(qmi8658_enable_accel(&s_imu, true), TAG, "Failed to enable accelerometer");

    s_initialized = true;
    ESP_LOGI(TAG, "QMI8658 initialized: range=4g, odr=250Hz");
    return ESP_OK;
}

esp_err_t imu_read_acc(imu_acc_data_t * data)
{
    uint8_t raw_data[6];

    ESP_RETURN_ON_FALSE(data != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid accelerometer data pointer");
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "QMI8658 is not initialized");
    ESP_RETURN_ON_ERROR(qmi8658_read_register(&s_imu, QMI8658_AX_L, raw_data, sizeof(raw_data)), TAG,
                        "Failed to read QMI8658 acceleration data");

    data->x = (int16_t)(((uint16_t)raw_data[1] << 8) | raw_data[0]);
    data->y = (int16_t)(((uint16_t)raw_data[3] << 8) | raw_data[2]);
    data->z = (int16_t)(((uint16_t)raw_data[5] << 8) | raw_data[4]);
    return ESP_OK;
}