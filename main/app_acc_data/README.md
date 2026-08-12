# Accelerometer Data App

Samples the QMI8658, publishes PMU and motion data to the LVGL model, and logs
accelerometer samples to the SD card.

Required board capabilities: display, SD storage, QMI8658 IMU, AXP2101 power
management, BOOT input, and a verified haptic output.

The QMI8658 is an on-board hardware dependency and is declared by the board.
The app source is still compiled by `main/CMakeLists.txt` during the independent
component migration tracked in the root `plan.md`.