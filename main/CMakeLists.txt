# BOARD selects the hardware profile; LVGL_PROJECT selects the application.
# Both are resolved in the top-level CMakeLists.txt.
set(LVGL_DIR "${CMAKE_SOURCE_DIR}/lvgl/${LVGL_PROJECT}")

set(MAIN_SRCS "app_${LVGL_PROJECT}.c")
set(MAIN_INCLUDE_DIRS "." "${CMAKE_SOURCE_DIR}/boards/${BOARD}" "${LVGL_DIR}/inc")

if(LVGL_PROJECT STREQUAL "slide_player")
    list(APPEND MAIN_SRCS
        ${LVGL_DIR}/src/slide_player.c)

elseif(LVGL_PROJECT STREQUAL "acc_data")
    list(APPEND MAIN_SRCS
        ${LVGL_DIR}/src/acc_data.c
        ${LVGL_DIR}/src/model.c
        imu.c
        pmu_power.cpp
        sd_acc_writer.c)

elseif(LVGL_PROJECT STREQUAL "battery_monitor")
    list(APPEND MAIN_SRCS
        ${LVGL_DIR}/src/battery_monitor.c
        ${LVGL_DIR}/src/model.c
        pmu_power.cpp)
endif()

# The `main` component automatically depends on every built component, so the
# BSP / IMU / PMU / driver components are available without an explicit REQUIRES
# list (managed dependencies are installed per board via boards/<board>/).
idf_component_register(
    SRCS ${MAIN_SRCS}
    INCLUDE_DIRS ${MAIN_INCLUDE_DIRS})

target_compile_definitions(${COMPONENT_LIB} PRIVATE LVGL_IN_USED=1)
