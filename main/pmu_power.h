#ifndef PMU_POWER_H
#define PMU_POWER_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t temperature_x10;
    uint32_t bat_voltage_mv;
    uint32_t vbus_voltage_mv;
    uint32_t system_voltage_mv;
    uint8_t bat_percent;
    uint8_t is_charging;
    uint8_t is_discharge;
    uint8_t is_standby;
    uint8_t is_vbus_in;
    uint8_t is_vbus_good;
    const char * charge_status;
} pmu_power_data_t;

esp_err_t pmu_power_init(void);
esp_err_t pmu_power_read_data(pmu_power_data_t * data);
esp_err_t pmu_power_poll_button(bool * pressed_edge, bool * released_edge);

#ifdef __cplusplus
}
#endif

#endif /* PMU_POWER_H */