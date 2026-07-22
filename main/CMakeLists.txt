set(LVGL_IN_USED 1)

set(MAIN_SOURCES main.c)
set(MAIN_INCLUDE_DIRS .)

if(LVGL_IN_USED)
    set(ACC_DATA_DIR ../acc_data)
    list(APPEND MAIN_SOURCES ${ACC_DATA_DIR}/src/acc_data.c)
    list(APPEND MAIN_SOURCES ${ACC_DATA_DIR}/src/model.c)
    list(APPEND MAIN_SOURCES task.c)
    list(APPEND MAIN_SOURCES imu.c)
    list(APPEND MAIN_SOURCES pmu_power.cpp)
    list(APPEND MAIN_INCLUDE_DIRS ${ACC_DATA_DIR}/inc)
endif()

idf_component_register(
    SRCS
        ${MAIN_SOURCES}
    INCLUDE_DIRS
        ${MAIN_INCLUDE_DIRS}
    REQUIRES
        esp_driver_gpio
        espressif__esp_lv_decoder
        esp_driver_i2c
        lvgl__lvgl
        XPowersLib
        waveshare__qmi8658
        waveshare__esp32_s3_touch_amoled_2_06)

if(LVGL_IN_USED)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE LVGL_IN_USED=1)
endif()
