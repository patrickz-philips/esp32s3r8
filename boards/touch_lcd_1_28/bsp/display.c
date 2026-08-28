#include "bsp/display.h"

#include "bsp/esp-bsp.h"
#include "board_config.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_lcd_gc9a01.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_touch_cst816s.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"

static const char *TAG = "bsp_display";

#define LCD_HOST            SPI2_HOST
// ponytail: GC9A01 accepts up to 80 MHz; 40 MHz is a conservative pre-device default.
#define LCD_PIXEL_CLOCK_HZ  (40 * 1000 * 1000)
#define LCD_CMD_BITS        8
#define LCD_PARAM_BITS      8
#define LCD_BITS_PER_PIXEL  16
#define LCD_DRAW_BUF_LINES  40

#define LEDC_BL_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_BL_TIMER       LEDC_TIMER_0
#define LEDC_BL_CHANNEL     LEDC_CHANNEL_0
#define LEDC_BL_DUTY_RES    LEDC_TIMER_10_BIT
#define LEDC_BL_FREQ_HZ     5000
#define LEDC_BL_MAX_DUTY    ((1 << 10) - 1)

static lv_display_t *s_disp;
static esp_lcd_panel_handle_t s_panel;
static esp_lcd_panel_io_handle_t s_panel_io;
static esp_lcd_touch_handle_t s_touch;

static esp_err_t backlight_init(void)
{
    const ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_BL_MODE,
        .duty_resolution = LEDC_BL_DUTY_RES,
        .timer_num = LEDC_BL_TIMER,
        .freq_hz = LEDC_BL_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_config), TAG, "Failed to configure backlight timer");

    const ledc_channel_config_t channel_config = {
        .gpio_num = BOARD_LCD_BL_GPIO,
        .speed_mode = LEDC_BL_MODE,
        .channel = LEDC_BL_CHANNEL,
        .timer_sel = LEDC_BL_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    return ledc_channel_config(&channel_config);
}

esp_err_t bsp_display_brightness_set(int brightness_percent)
{
    if (brightness_percent < 0) {
        brightness_percent = 0;
    } else if (brightness_percent > 100) {
        brightness_percent = 100;
    }

    const uint32_t duty = ((uint32_t)LEDC_BL_MAX_DUTY * (uint32_t)brightness_percent) / 100U;
    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_BL_MODE, LEDC_BL_CHANNEL, duty), TAG, "Failed to set backlight duty");
    return ledc_update_duty(LEDC_BL_MODE, LEDC_BL_CHANNEL);
}

static esp_err_t panel_init(void)
{
    const spi_bus_config_t bus_config = GC9A01_PANEL_BUS_SPI_CONFIG(
        BOARD_LCD_SCLK_GPIO, BOARD_LCD_MOSI_GPIO,
        BOARD_LCD_H_RES * LCD_DRAW_BUF_LINES * sizeof(uint16_t));
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_HOST, &bus_config, SPI_DMA_CH_AUTO), TAG, "Failed to init SPI bus");

    const esp_lcd_panel_io_spi_config_t io_config =
        GC9A01_PANEL_IO_SPI_CONFIG(BOARD_LCD_CS_GPIO, BOARD_LCD_DC_GPIO, NULL, NULL);
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &s_panel_io),
        TAG, "Failed to create panel IO");

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BOARD_LCD_RST_GPIO,
        // BGR order + color inversion match the Waveshare GC9A01 reference
        // (TFT_eSPI rotation 0 = MADCTL BGR, INVON).
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = LCD_BITS_PER_PIXEL,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_gc9a01(s_panel_io, &panel_config, &s_panel), TAG, "Failed to create GC9A01");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel), TAG, "Failed to reset panel");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel), TAG, "Failed to init panel");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(s_panel, true), TAG, "Failed to invert color");
    // NOTE: Do not call esp_lcd_panel_mirror()/swap_xy() here. esp_lvgl_port
    // re-applies mirror/swap from lvgl_port_display_cfg_t.rotation at
    // lvgl_port_add_disp() time, which would override anything set here.
    // Orientation is configured via the display rotation config below.
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel, true), TAG, "Failed to turn display on");
    return ESP_OK;
}

static esp_err_t touch_init(void)
{
    i2c_master_bus_handle_t i2c_bus = bsp_i2c_get_handle();
    ESP_RETURN_ON_FALSE(i2c_bus != NULL, ESP_ERR_INVALID_STATE, TAG, "Shared I2C bus is not initialized");

    esp_lcd_panel_io_handle_t touch_io = NULL;
    const esp_lcd_panel_io_i2c_config_t touch_io_config = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_bus, &touch_io_config, &touch_io), TAG, "Failed to create touch IO");

    const esp_lcd_touch_config_t touch_config = {
        .x_max = BOARD_LCD_H_RES,
        .y_max = BOARD_LCD_V_RES,
        .rst_gpio_num = BOARD_TOUCH_RST_GPIO,
        .int_gpio_num = BOARD_TOUCH_INT_GPIO,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            // Match the display X mirror so touch tracks the visible image.
            .mirror_x = 1,
            .mirror_y = 0,
        },
    };
    return esp_lcd_touch_new_i2c_cst816s(touch_io, &touch_config, &s_touch);
}

lv_display_t *bsp_display_start(void)
{
    if (s_disp != NULL) {
        return s_disp;
    }

    if (backlight_init() != ESP_OK) {
        ESP_LOGE(TAG, "Backlight init failed");
        return NULL;
    }
    if (panel_init() != ESP_OK) {
        ESP_LOGE(TAG, "Panel init failed");
        return NULL;
    }

    const lvgl_port_cfg_t port_config = ESP_LVGL_PORT_INIT_CONFIG();
    if (lvgl_port_init(&port_config) != ESP_OK) {
        ESP_LOGE(TAG, "LVGL port init failed");
        return NULL;
    }

    const lvgl_port_display_cfg_t display_config = {
        .io_handle = s_panel_io,
        .panel_handle = s_panel,
        .buffer_size = BOARD_LCD_H_RES * LCD_DRAW_BUF_LINES,
        .double_buffer = true,
        .hres = BOARD_LCD_H_RES,
        .vres = BOARD_LCD_V_RES,
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            // esp_lvgl_port owns the panel mirror/swap; setting it here is the
            // only place that sticks. X is mirrored to correct the image.
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
            .swap_bytes = true,
        },
    };
    s_disp = lvgl_port_add_disp(&display_config);
    if (s_disp == NULL) {
        ESP_LOGE(TAG, "Failed to add LVGL display");
        return NULL;
    }

    if (touch_init() != ESP_OK) {
        ESP_LOGE(TAG, "Touch init failed");
        return NULL;
    }
    const lvgl_port_touch_cfg_t touch_config = {
        .disp = s_disp,
        .handle = s_touch,
    };
    if (lvgl_port_add_touch(&touch_config) == NULL) {
        ESP_LOGE(TAG, "Failed to add LVGL touch");
        return NULL;
    }

    if (bsp_display_brightness_set(100) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to enable backlight");
    }

    ESP_LOGI(TAG, "Display started: %dx%d GC9A01 + CST816S", BOARD_LCD_H_RES, BOARD_LCD_V_RES);
    return s_disp;
}

esp_err_t bsp_display_lock(uint32_t timeout_ms)
{
    // esp_lvgl_port treats 0 as "wait forever"; map the BSP's UINT32_MAX to it.
    const uint32_t port_timeout_ms = (timeout_ms == UINT32_MAX) ? 0U : timeout_ms;
    return lvgl_port_lock(port_timeout_ms) ? ESP_OK : ESP_ERR_TIMEOUT;
}

void bsp_display_unlock(void)
{
    lvgl_port_unlock();
}
