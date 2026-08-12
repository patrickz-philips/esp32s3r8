if(NOT DEFINED ENV{SELECTED_BSP_COMPONENT})
    message(FATAL_ERROR "Selected BSP component was not exported by the project")
endif()

idf_component_register(
    SRCS "src/pmu_power.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_common
    PRIV_REQUIRES
        XPowersLib
        esp_driver_i2c
        freertos
        $ENV{SELECTED_BSP_COMPONENT})