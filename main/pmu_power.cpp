#include "pmu_power.h"

#include <cstring>

#include "bsp/esp-bsp.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"

namespace {

constexpr char TAG[] = "pmu_power";
constexpr uint32_t PMU_I2C_FREQ_HZ = 400000U;
constexpr int PMU_I2C_TIMEOUT_MS = 1000;
constexpr uint32_t PMU_MUTEX_TIMEOUT_MS = 50U;

XPowersPMU pmu;
i2c_master_dev_handle_t pmu_dev_handle = nullptr;
SemaphoreHandle_t pmu_mutex = nullptr;

int pmu_register_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t * data, uint8_t len)
{
    (void)dev_addr;

    esp_err_t ret = i2c_master_transmit_receive(pmu_dev_handle, &reg_addr, 1, data, len, PMU_I2C_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PMU read reg 0x%02x failed: %s", reg_addr, esp_err_to_name(ret));
        return -1;
    }

    return 0;
}

int pmu_register_write_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t * data, uint8_t len)
{
    (void)dev_addr;

    uint8_t buffer[16];
    if (len + 1U > sizeof(buffer)) {
        ESP_LOGE(TAG, "PMU write too long: %u", (unsigned int)len);
        return -1;
    }

    buffer[0] = reg_addr;
    if (len > 0U) {
        std::memcpy(&buffer[1], data, len);
    }

    esp_err_t ret = i2c_master_transmit(pmu_dev_handle, buffer, len + 1U, PMU_I2C_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PMU write reg 0x%02x failed: %s", reg_addr, esp_err_to_name(ret));
        return -1;
    }

    return 0;
}

const char * charge_status_text(uint8_t status)
{
    switch (status) {
        case XPOWERS_AXP2101_CHG_TRI_STATE:
            return "tri_charge";
        case XPOWERS_AXP2101_CHG_PRE_STATE:
            return "pre_charge";
        case XPOWERS_AXP2101_CHG_CC_STATE:
            return "constant charge";
        case XPOWERS_AXP2101_CHG_CV_STATE:
            return "constant voltage";
        case XPOWERS_AXP2101_CHG_DONE_STATE:
            return "charge done";
        case XPOWERS_AXP2101_CHG_STOP_STATE:
            return "not charging";
        default:
            return "unknown";
    }
}

uint32_t clamp_non_negative(int value)
{
    return value > 0 ? (uint32_t)value : 0U;
}

int32_t temperature_to_x10(float temperature)
{
    return (int32_t)((temperature * 10.0f) + (temperature >= 0.0f ? 0.5f : -0.5f));
}

bool take_pmu_mutex(TickType_t timeout_ticks)
{
    return pmu_mutex != nullptr && xSemaphoreTake(pmu_mutex, timeout_ticks) == pdTRUE;
}

void give_pmu_mutex(void)
{
    if (pmu_mutex != nullptr) {
        xSemaphoreGive(pmu_mutex);
    }
}

esp_err_t pmu_i2c_device_init(void)
{
    if (pmu_dev_handle != nullptr) {
        return ESP_OK;
    }

    i2c_master_bus_handle_t bus_handle = bsp_i2c_get_handle();
    if (bus_handle == nullptr) {
        return ESP_FAIL;
    }

    i2c_device_config_t dev_config = {};
    dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_config.device_address = AXP2101_SLAVE_ADDRESS;
    dev_config.scl_speed_hz = PMU_I2C_FREQ_HZ;

    return i2c_master_bus_add_device(bus_handle, &dev_config, &pmu_dev_handle);
}

esp_err_t pmu_chip_init(void)
{
    if (!pmu.begin(AXP2101_SLAVE_ADDRESS, pmu_register_read, pmu_register_write_byte)) {
        ESP_LOGE(TAG, "Init AXP2101 failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Init AXP2101 success");

    pmu.clearIrqStatus();
    pmu.enableTemperatureMeasure();
    pmu.enableBattDetection();
    pmu.enableVbusVoltageMeasure();
    pmu.enableBattVoltageMeasure();
    pmu.enableSystemVoltageMeasure();
    pmu.disableTSPinMeasure();

    pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    pmu.clearIrqStatus();
    pmu.enableIRQ(XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ | XPOWERS_AXP2101_PKEY_POSITIVE_IRQ);
    pmu.disableLongPressShutdown();

    pmu.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
    pmu.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_400MA);
    pmu.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);
    pmu.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);

    return ESP_OK;
}

} // namespace

extern "C" esp_err_t pmu_power_init(void)
{
    if (pmu_mutex == nullptr) {
        pmu_mutex = xSemaphoreCreateMutex();
        ESP_RETURN_ON_FALSE(pmu_mutex != nullptr, ESP_ERR_NO_MEM, TAG, "Failed to create PMU mutex");
    }

    ESP_RETURN_ON_ERROR(pmu_i2c_device_init(), TAG, "Failed to add PMU I2C device");
    return pmu_chip_init();
}

extern "C" esp_err_t pmu_power_read_data(battery_monitor_data_t * data)
{
    ESP_RETURN_ON_FALSE(data != nullptr, ESP_ERR_INVALID_ARG, TAG, "Invalid PMU data pointer");

    if (!take_pmu_mutex(pdMS_TO_TICKS(PMU_MUTEX_TIMEOUT_MS))) {
        return ESP_ERR_TIMEOUT;
    }

    data->temperature_x10 = temperature_to_x10(pmu.getTemperature());
    data->bat_voltage_mv = clamp_non_negative(pmu.getBattVoltage());
    data->vbus_voltage_mv = clamp_non_negative(pmu.getVbusVoltage());
    data->system_voltage_mv = clamp_non_negative(pmu.getSystemVoltage());
    data->bat_percent = pmu.isBatteryConnect() ? (uint8_t)pmu.getBatteryPercent() : 0U;
    data->is_charging = pmu.isCharging() ? 1U : 0U;
    data->is_discharge = pmu.isDischarge() ? 1U : 0U;
    data->is_standby = pmu.isStandby() ? 1U : 0U;
    data->is_vbus_in = pmu.isVbusIn() ? 1U : 0U;
    data->is_vbus_good = pmu.isVbusGood() ? 1U : 0U;
    data->charge_status = charge_status_text(pmu.getChargerStatus());

    give_pmu_mutex();
    return ESP_OK;
}

extern "C" esp_err_t pmu_power_poll_button(bool * pressed_edge, bool * released_edge)
{
    ESP_RETURN_ON_FALSE(pressed_edge != nullptr, ESP_ERR_INVALID_ARG, TAG, "Invalid pressed_edge pointer");
    ESP_RETURN_ON_FALSE(released_edge != nullptr, ESP_ERR_INVALID_ARG, TAG, "Invalid released_edge pointer");

    *pressed_edge = false;
    *released_edge = false;

    if (!take_pmu_mutex(pdMS_TO_TICKS(PMU_MUTEX_TIMEOUT_MS))) {
        return ESP_ERR_TIMEOUT;
    }

    const uint64_t irq_status = pmu.getIrqStatus();
    if (irq_status != 0U) {
        *pressed_edge = pmu.isPekeyNegativeIrq();
        *released_edge = pmu.isPekeyPositiveIrq();
        pmu.clearIrqStatus();
    }

    give_pmu_mutex();
    return ESP_OK;
}