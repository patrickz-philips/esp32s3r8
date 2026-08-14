# ESP LVGL Adapter Override

This directory vendors `espressif/esp_lvgl_adapter` 0.6.3 from ESP Component
Registry commit `db9f2f48527f9a98a87e0ca741c8dacd31814f42`.

For LVGL 9, GPIO TE mode uses partial rendering with an internal draw buffer
bounded by `profile.buffer_height`. All dirty-area stripes in one LVGL refresh
share one TE wait, then transfer sequentially before the draw buffer is reused.
This keeps GIF updates local and avoids PSRAM-backed SPI DMA bounce buffers.