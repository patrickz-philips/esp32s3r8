set(LVGL_IN_USED 1)

set(MAIN_SOURCES main.c)
set(MAIN_INCLUDE_DIRS .)

if(LVGL_IN_USED)
    set(BATTERY_MONITOR_DIR ../battery_monitor)
    list(APPEND MAIN_SOURCES ${BATTERY_MONITOR_DIR}/src/battery_monitor.c)
    list(APPEND MAIN_SOURCES pmu_power.cpp)
    list(APPEND MAIN_INCLUDE_DIRS ${BATTERY_MONITOR_DIR}/inc)
endif()

idf_component_register(
    SRCS
        ${MAIN_SOURCES}
    INCLUDE_DIRS
        ${MAIN_INCLUDE_DIRS}
    REQUIRES
        espressif__esp_lv_decoder
        esp_driver_i2c
        lvgl__lvgl
        XPowersLib
        waveshare__esp32_s3_touch_amoled_2_06)

if(LVGL_IN_USED)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE LVGL_IN_USED=1)
endif()
