# Accelerometer Data App

Samples the QMI8658, publishes PMU and motion data to the LVGL model, and logs
accelerometer samples to the SD card.

## Required Capabilities

The app requires a display, SD storage, QMI8658 IMU, AXP2101 PMU, and GPIO0
BOOT input. Haptic feedback is optional and is compiled only when the selected
board declares a verified haptic capability. The 2.06 profile enables GPIO18;
the 1.75 profile does not create a haptic task or configure a haptic GPIO.

The QMI8658 and BSP are board-owned dependencies. `app_acc_data` is an
independent selected component that owns its entry point, IMU adapter, SD writer,
FreeRTOS tasks, and the acc-data LVGL bridge.

## Initialization

Initialization starts the BSP display, creates the LVGL UI and model queues,
initializes the SD writer, PMU, IMU, and BOOT input, then creates the worker
tasks. A required initialization failure prevents later stages from starting.

## Task Model

| Task | Stack | Priority | Responsibility |
|:-----|:------|:---------|:---------------|
| `taskIMU` | 4096 bytes | 2 | Sample acceleration every 20 ms and post it to the model and SD writer |
| `taskPMU` | 4096 bytes | 1 | Publish battery telemetry every second |
| `taskPress` | 4096 bytes | 2 | Debounce GPIO0 and poll PMU power-key events |
| `taskSDAcc` | 4096 bytes | 1 | Mount SD on demand and write buffered CSV samples |
| `taskHaptic` | 2048 bytes | 3 | Pulse the board-declared haptic output for 40 ms; 2.06 only |

The BSP owns the LVGL task. UI objects are initialized while the display lock is
held, and asynchronous tasks communicate through typed model queues. On the
2.06 profile, the LVGL task is pinned to core 0 to preserve the original display
refresh scheduling.

## LVGL Model Contract

The UI receives acceleration, PMU, button, SD completion, and missing-card
events through `model.h`. Its top-level layout reads the active LVGL display's
pixel resolution, so it does not depend on one board's panel dimensions or BSP
headers.

## Device Checks

Builds validate component wiring only. On each board, verify chart layout,
BOOT-controlled recording, PMU shutdown, SD insertion/removal, and CSV content.