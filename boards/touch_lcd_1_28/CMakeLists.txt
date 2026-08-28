# Board component for Waveshare ESP32-S3-Touch-LCD-1.28.
# Unlike the AMOLED boards (which consume a vendor BSP managed component), this
# board owns its BSP: a GC9A01 SPI panel + CST816S I2C touch driven through
# esp_lvgl_port. Only the selected board directory is added via
# EXTRA_COMPONENT_DIRS, so the component manager installs just these deps.
idf_component_register(
    SRCS
        "bsp/display.c"
        "bsp/i2c.c"
    INCLUDE_DIRS
        "."
        "bsp"
    REQUIRES
        lvgl__lvgl
        esp_driver_i2c
    PRIV_REQUIRES
        esp_lcd
        esp_driver_spi
        esp_driver_ledc
        esp_driver_gpio
        espressif__esp_lvgl_port
        espressif__esp_lcd_gc9a01
        espressif__esp_lcd_touch_cst816s)
