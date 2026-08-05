#pragma once

// Waveshare ESP32-S3 Touch AMOLED 1.75" board profile.
#define BOARD_NAME              "Waveshare ESP32-S3 Touch AMOLED 1.75\""

// Capability flags (used by main/ to compile only the relevant features).
#define BOARD_HAS_SD            1
#define BOARD_HAS_IMU           0
#define BOARD_HAS_PMU           0
#define BOARD_FEATURE_SLIDE_PLAYER 1
#define BOARD_FEATURE_ACC_DATA     0

// SD card is mounted via the board BSP (bsp_sdcard_mount); wiring lives there.
