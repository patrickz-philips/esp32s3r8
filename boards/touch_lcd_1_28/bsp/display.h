#pragma once

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_LCD_H_RES 240
#define BSP_LCD_V_RES 240

/**
 * @brief Initialize the display (SPI + GC9A01 + CST816S touch) and start LVGL.
 *
 * @return Pointer to the LVGL display, or NULL on error.
 */
lv_display_t *bsp_display_start(void);

/**
 * @brief Set the LCD backlight brightness (0-100%).
 */
esp_err_t bsp_display_brightness_set(int brightness_percent);

/**
 * @brief Take the LVGL mutex before calling any lv_* API from another thread.
 *
 * @param timeout_ms Timeout in ms; UINT32_MAX blocks indefinitely.
 * @return ESP_OK when the mutex was taken.
 */
esp_err_t bsp_display_lock(uint32_t timeout_ms);

/**
 * @brief Release the LVGL mutex.
 */
void bsp_display_unlock(void);

#ifdef __cplusplus
}
#endif
 