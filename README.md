# ESP32-S3 LVGL Slide Demo (v9)

This project is an ESP-IDF + LVGL demo for the Waveshare ESP32-S3 AMOLED 1.75 board.
It mounts an SD card, loads PNG slide images at runtime, and switches slides with touch gestures.

## Architecture Overview

The project is organized as a small layered application:

1. Platform layer (BSP and board drivers)
- Board init and display bring-up are handled by the Waveshare BSP component.
- SD card is mounted over SDSPI.

2. Runtime services
- LVGL image decoder is initialized via `esp_lv_decoder`.
- Filesystem path for UI assets is mapped to `A:/sdcard` (LVGL path style).

3. UI / feature layer (`slide_player`)
- Creates the LVGL screen and widgets.
- Handles gesture events (left/right swipe).
- Builds image paths like `A:/sdcard/1.png` ... `A:/sdcard/32.png`.

## Runtime Flow

1. `app_main()` starts board display and acquires the display lock.
2. `slide_player_runtime_init()` mounts SD card and initializes LVGL decoder.
3. `slide_player_ui_init()` builds the UI and displays the first slide.
4. Gesture callback updates slide index and refreshes image source.

## Project Structure

```text
.
|-- CMakeLists.txt                # Project CMake entry
|-- main/
|   |-- CMakeLists.txt            # Registers main + slide_player/src
|   |-- idf_component.yml         # Component dependencies
|   `-- main.c                    # App entry and runtime init
|-- slide_player/
|   |-- inc/
|   |   `-- slide_player.h        # Public UI API
|   |-- src/
|   |   `-- slide_player.c        # UI logic and gesture handling
|   `-- assets/
|       |-- *.png                 # Runtime image assets on SD card
|       `-- convert_image.py      # PNG-to-C helper script (optional)
|-- sdkconfig                     # Current project configuration
`-- dependencies.lock             # Locked component versions
```

## Key Dependencies

- ESP-IDF: `5.5.4`
- LVGL: `9.4.x`
- Waveshare board support: `waveshare/esp32_s3_touch_amoled_1_75`
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

## Slide Asset Requirements

- Place slide images on SD card under `/sdcard`.
- Current code expects numbered PNG files: `1.png` ... `32.png`.
- Gesture behavior:
  - Swipe left: next slide
  - Swipe right: previous slide

## Current Design Notes

- UI updates are gesture-driven and run on LVGL event callbacks.
- Performance logs are emitted on each slide switch to help profile decode and memory usage.
- Asset C files in `slide_player/assets/*.c` are not compiled by default in the current CMake setup; runtime loading is PNG-from-SD based.

## Future Improvement Ideas

- Add rollback cleanup if any runtime init step fails.
- Replace hardcoded slide count with SD directory scan.
- Move machine-specific `.vscode` settings to user-local config.
- Reduce global warning suppression and scope it to specific files only.
