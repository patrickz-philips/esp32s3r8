#pragma once

// Waveshare ESP32-S3 Touch AMOLED 2.06" board profile.
#define BOARD_NAME              "Waveshare ESP32-S3 Touch AMOLED 2.06\""

// Physical board capabilities. App compatibility lives in supported_apps.txt.
#define BOARD_HAS_SD            1
#define BOARD_HAS_TOUCH         1
#define BOARD_HAS_AUDIO         1
#define BOARD_HAS_IMU           1
#define BOARD_HAS_PMU           1
#define BOARD_HAS_RTC           1
#define BOARD_HAS_HAPTIC        1
#define BOARD_HAPTIC_GPIO       18

// Current project integration status, not raw hardware capability.
#define BOARD_FEATURE_SLIDE_PLAYER 1
#define BOARD_FEATURE_ACC_DATA     1
