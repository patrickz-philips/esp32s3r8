# ESP32S3 Waveshare 2.06 LVGL Platform

This project is the hardware platform code for the ESP32S3 Waveshare 2.06 development board.
It provides an ESP-IDF + LVGL base for board bring-up and the `battery_monitor` LVGL feature UI.

## Architecture Overview

The project is organized as a small layered application:

1. Platform layer (BSP and board drivers)
- Board init and display bring-up are handled by the Waveshare BSP component.
- SD card support is kept for features that load files from storage.

2. Runtime services
- LVGL is provided by the managed component dependency.
- The BSP display lock is held while the UI is created.
- SD card and LVGL decoder dependencies remain available for hardware-platform features, but they are not initialized in the default boot path.

3. UI / feature layer
- `battery_monitor` shows battery voltage, VBUS voltage, system voltage, percentage, and charge state.
- `battery_monitor` is compiled and started only when `LVGL_IN_USED` is enabled in `main/CMakeLists.txt`.

## Runtime Flow

1. `app_main()` starts board display and acquires the display lock.
2. When `LVGL_IN_USED` is enabled, `battery_monitor_ui_init()` creates the LVGL UI.
3. Gesture callbacks update the active monitor page.

## Project Structure

```text
.
|-- CMakeLists.txt                # Project CMake entry
|-- main/
|   |-- CMakeLists.txt            # Registers main + feature UI sources
|   |-- idf_component.yml         # Component dependencies
|   `-- main.c                    # App entry and runtime init
|-- battery_monitor/
|   |-- inc/
|   |   `-- battery_monitor.h     # Public UI API and data type
|   |-- src/
|   |   `-- battery_monitor.c     # UI logic and gesture handling
|-- sdkconfig                     # Current project configuration
`-- dependencies.lock             # Locked component versions
```

## Key Dependencies

- ESP-IDF: `5.5.4`
- LVGL: `9.4.x`
- Waveshare board support: `waveshare/esp32_s3_touch_amoled_2_06`
- LVGL decoder: `espressif/esp_lv_decoder`

See `main/idf_component.yml` and `dependencies.lock` for exact dependency definitions.

## Build and Flash

Use ESP-IDF environment first, then build/flash as usual:

```bash
idf.py build
idf.py -p <PORT> flash monitor
```

Example on Windows PowerShell (if needed):

```powershell
$env:IDF_PATH = "C:\Espressif\frameworks\esp-idf-v5.5.4"
idf.py build
idf.py -p COM3 flash monitor
```

## Gesture Behavior

- `battery_monitor`: swipe left/right to switch monitor pages.

## Current Design Notes

- The project root represents the board hardware platform, not a single feature app.
- UI updates are gesture-driven and run on LVGL event callbacks.
- `battery_monitor_set_data()` can be used to replace the default sample values.
- SD card and LVGL decoder support remains in the project dependencies, but the default boot path does not depend on it.
- `LVGL_IN_USED` is the build-time switch for including the current LVGL UI module.

## Future Improvement Ideas

- Move machine-specific `.vscode` settings to user-local config.
- Reduce global warning suppression and scope it to specific files only.
