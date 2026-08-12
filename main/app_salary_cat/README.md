# Salary Cat App

Salary Cat loops a GIF and an MP3 file from the SD card. Vertical touch gestures
adjust speaker volume, and a three-second PMU power-key press requests a
controlled shutdown.

## Required Capabilities

| Capability | Use |
|:-----------|:----|
| Display and touch | GIF playback and volume gestures |
| SD storage | Reads `/sdcard/cat.GIF` and `/sdcard/music.mp3` |
| Speaker codec | Mono PCM output through the BSP codec API |
| AXP2101 PMIC | Power-key polling and software shutdown |

The current compatibility source enables this app for `amoled_175` and
`amoled_206`.

## Component Ownership

`app_salary_cat` owns its entry point, fixed asset paths, FreeRTOS tasks, MP3
decoder adapter, and the salary-cat LVGL bridge. Its app-only Component Manager
dependency is `chmorgan/esp-libhelix-mp3`.

The reusable `pmu_power` component owns AXP2101 access for this app and other
PMU consumers. Board initialization remains behind the selected BSP APIs.

## Initialization and Failure Handling

Initialization proceeds in this order:

1. Start the BSP display and touch stack.
2. Mount the SD card.
3. Initialize board-specific audio control, the speaker codec, and Helix decoder.
4. Initialize the PMU service.
5. Build the LVGL UI and cache the GIF in RAM when possible.
6. Start the audio and power-key tasks, then begin GIF playback.

The app does not start playback after a required subsystem fails. Failures after
SD mount roll back the audio, PMU, SD, and any app tasks already created. GIF
caching is optional; playback falls back to the SD file when allocation or file
loading fails.

On `amoled_175`, audio initialization first acquires the BSP-owned TCA9554 and
then creates the ES8311 speaker codec through the BSP. The BSP retains ownership
of GPIO46 PA control. The app does not infer or drive an undocumented TCA9554
codec-enable bit.

## Display Configuration

The app enables `CONFIG_LV_USE_GIF` through `sdkconfig.app`. The 1.75 profile
uses the BSP's 50-row PSRAM double buffer, while the 2.06 profile uses a 60-row
buffer. Neither app integration path changes the board BSP display communication
frequency.

## Task Model

| Task | Stack | Priority | Responsibility |
|:-----|:------|:---------|:---------------|
| `salary_audio` | 8192 bytes | 5 | Decode and replay the MP3; apply pending volume changes |
| `salary_power` | 4096 bytes | 3 | Poll PMU key edges every 20 ms and detect a 3 s press |

LVGL itself runs in the BSP-managed task. The app holds the BSP display lock for
UI creation and playback changes; file decoding and PMU polling never run while
that lock is held.

## LVGL Model Contract

The UI receives a `salary_cat_model_t` callback interface from the app. A swipe
requests a relative volume change through that interface and displays the
returned volume. The LVGL layer does not include codec, PMU, SD, or BSP service
headers.

## Device Checks

The build validates component wiring only. On the physical board, verify GIF and
MP3 looping, swipe volume changes, SD fallback behavior, and three-second power
key shutdown.