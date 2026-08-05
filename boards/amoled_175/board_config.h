#pragma once

// Waveshare ESP32-S3 Touch AMOLED 1.75" board profile.
#define BOARD_NAME              "Waveshare ESP32-S3 Touch AMOLED 1.75\""

// Capability flags (used by main/ to compile only the relevant features).
#define BOARD_HAS_SD            1
#define BOARD_HAS_IMU           0
#define BOARD_HAS_PMU           0
#define BOARD_FEATURE_SLIDE_PLAYER 1
#define BOARD_FEATURE_ACC_DATA     0

// SD card is wired over SPI on this board.
#define BOARD_SD_MOUNT_POINT    "/sdcard"
#define BOARD_SD_PIN_CS         41
#define BOARD_SD_PIN_MOSI       1
#define BOARD_SD_PIN_MISO       3
#define BOARD_SD_PIN_SCK        2
