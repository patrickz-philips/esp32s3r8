# BSP: Waveshare ESP32-S3-Touch-AMOLED-2.06

## Local Override

This directory is the version-controlled project override for Waveshare BSP
`2.0.0`, local revision 1. It preserves the public BSP API while replacing the
SH8601 display abstraction with the physical CO5300 controller and registering
the QSPI display through `esp_lvgl_adapter` GPIO TE synchronization on GPIO13.
The project selects it through `override_path` from
`boards/amoled_206/idf_component.yml` when the 2.06-inch board is selected.

[![Component Registry](https://components.espressif.com/components/waveshare/esp32_s3_touch_amoled_2_06/badge.svg)](https://components.espressif.com/components/waveshare/esp32_s3_touch_amoled_2_06)

The ESP32-S3-Touch-AMOLED-2.06 is a 2.06-inch 410×502 capacitive touch development board designed by waveshare electronics.

|                            HW version                            | BSP Version |
|:----------------------------------------------------------------:| :---------: |
| [V1.0](http://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-2.06) |      ^1     |