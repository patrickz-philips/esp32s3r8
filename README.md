# ESP32S3 Waveshare 2.06 LVGL Platform

This project provides the hardware platform code for the Waveshare ESP32-S3 Touch AMOLED 2.06 board. It uses ESP-IDF and LVGL as a reusable base for board bring-up, peripheral services, and feature UI modules.

The current `acc_data` module is one feature built on this platform. It displays raw three-axis accelerometer samples, but it does not define the purpose of the root project.

## Architecture Overview

The project is organized into three layers:

1. Hardware platform

- The Waveshare BSP handles board initialization and display bring-up.
- `main/imu.c` configures the QMI8658 accelerometer at 4 g and 250 Hz.
- `main/pmu_power.cpp` configures AXP2101 charging protection and reads PMU and button state.
- `main/task.c` runs the IMU, PMU, button, and haptic hardware tasks.

2. Runtime services

- ESP-IDF provides peripheral drivers and task scheduling.
- LVGL and the Waveshare BSP provide the display runtime.
- Hardware producers publish state without directly manipulating LVGL objects.

3. Feature modules

- Feature UI modules live outside `main/` and consume services exposed by the platform.
- The current `acc_data/` module owns the accelerometer chart widgets and rendering.
- `acc_data/src/model.c` queues accelerometer, PMU, and button events and applies them from the LVGL timer context.

## Current Feature: Accelerometer Data

The IMU task polls acceleration every 20 ms and publishes each successful raw sample through `model_post_acc_data()`. A single-slot overwrite queue keeps only the latest accelerometer sample, so the LVGL model never replays stale data.

PMU data is retained in the model for future UI use. GPIO0 and PWRON button events are queued and reserved for future behavior.

```text
IMU task/driver -> model_post_acc_data() -> model queue -> acc_data_push_sample() -> LVGL chart
```

The `LVGL_IN_USED` switch in `main/CMakeLists.txt` controls whether the current LVGL feature and its supporting hardware tasks are included.

### SD Card Recording

While no recording session is active, the SD writer task checks the board microSD card every 10 seconds and retains its mounted status. When mounted, it scans files named `acc_<index>.csv`. Each new file uses the next index after the largest existing one. A CSV starts with the `x,y,z` header and contains exactly 3000 raw accelerometer LSB samples collected at the IMU task's 20 ms period.

Pressing the Boot button starts recording and the green stopwatch indicator. If no card is mounted, recording is cancelled, the stopwatch and indicator stop, and `sdcard x` is shown. Every 3000 samples, the writer closes the current CSV and immediately continues in the next numbered CSV. Pressing Boot again requests a wait stop, stops the indicator, and shows `saving`; the writer completes the current 3000-sample CSV before ending the recording session and changing the status to `finish`. A write failure changes the status to `error`.

## Project Structure

```text
.
|-- CMakeLists.txt
|-- main/
|   |-- CMakeLists.txt
|   |-- main.c
|   |-- imu.c
|   |-- imu.h
|   |-- pmu_power.cpp
|   |-- pmu_power.h
|   |-- task.c
|   `-- task.h
`-- acc_data/
	|-- inc/
	|   |-- acc_data.h
	|   `-- model.h
	`-- src/
		|-- acc_data.c
		`-- model.c
```

The existing `build/` directory must be regenerated after moving or renaming the project because ESP-IDF and Ninja cache absolute source paths.
