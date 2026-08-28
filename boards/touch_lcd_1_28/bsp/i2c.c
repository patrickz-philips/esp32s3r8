#include "bsp/esp-bsp.h"

#include "board_config.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "bsp_i2c";

// Shared I2C master bus for the CST816S touch controller and QMI8658 IMU.
static i2c_master_bus_handle_t s_i2c_bus;

esp_err_t bsp_i2c_init(void)
{
    if (s_i2c_bus != NULL) {
        return ESP_OK;
    }

    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = BOARD_I2C_SDA_GPIO,
        .scl_io_num = BOARD_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &s_i2c_bus), TAG, "Failed to create I2C master bus");
    ESP_LOGI(TAG, "Shared I2C bus ready: SDA=%d, SCL=%d", BOARD_I2C_SDA_GPIO, BOARD_I2C_SCL_GPIO);
    return ESP_OK;
}

esp_err_t bsp_i2c_deinit(void)
{
    if (s_i2c_bus == NULL) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(i2c_del_master_bus(s_i2c_bus), TAG, "Failed to delete I2C master bus");
    s_i2c_bus = NULL;
    return ESP_OK;
}

i2c_master_bus_handle_t bsp_i2c_get_handle(void)
{
    if (s_i2c_bus == NULL && bsp_i2c_init() != ESP_OK) {
        return NULL;
    }
    return s_i2c_bus;
}
