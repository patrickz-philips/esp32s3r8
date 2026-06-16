#ifndef PMU_POWER_H
#define PMU_POWER_H

#include <stdbool.h>

#include "battery_monitor.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t pmu_power_init(void);
esp_err_t pmu_power_read_data(battery_monitor_data_t * data);
esp_err_t pmu_power_poll_button(bool * pressed_edge, bool * released_edge);

#ifdef __cplusplus
}
#endif

#endif /* PMU_POWER_H */
