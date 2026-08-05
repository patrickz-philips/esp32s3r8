#ifndef APP_IMU_H
#define APP_IMU_H

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} imu_acc_data_t;

esp_err_t imu_init(void);
esp_err_t imu_read_acc(imu_acc_data_t * data);

#ifdef __cplusplus
}
#endif

#endif /* APP_IMU_H */