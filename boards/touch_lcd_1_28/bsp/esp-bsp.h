#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "lvgl.h"

#include "bsp/display.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the shared I2C master bus (touch + IMU).
 */
esp_err_t bsp_i2c_init(void);

/**
 * @brief De-initialize the shared I2C master bus.
 */
esp_err_t bsp_i2c_deinit(void);

/**
 * @brief Get the shared I2C master bus handle, initializing it on first use.
 */
i2c_master_bus_handle_t bsp_i2c_get_handle(void);

#ifdef __cplusplus
}
#endif
