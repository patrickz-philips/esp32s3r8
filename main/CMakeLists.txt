# BOARD selects the hardware profile; LVGL_PROJECT selects the application.
# Both are resolved in the top-level CMakeLists.txt.
set(LVGL_DIR "${CMAKE_SOURCE_DIR}/lvgl/${LVGL_PROJECT}")

# Each app's entry point lives in its own folder: main/app_<project>/app_<project>.c
set(MAIN_SRCS "")
set(MAIN_INCLUDE_DIRS "." "${CMAKE_SOURCE_DIR}/boards/${BOARD}" "${LVGL_DIR}/inc")

if(LVGL_PROJECT STREQUAL "slide_player")
    # The selected app component owns its entry point, UI bridge, and storage pipeline.

elseif(LVGL_PROJECT STREQUAL "salary_cat")
    # The selected app component owns its entry point, UI bridge, and audio.

elseif(LVGL_PROJECT STREQUAL "acc_data")
    # The selected app component owns its entry point, UI bridge, IMU, and SD writer.

elseif(LVGL_PROJECT STREQUAL "battery_monitor")
    list(APPEND MAIN_SRCS
        app_battery_monitor/app_battery_monitor.c
        ${LVGL_DIR}/src/battery_monitor.c
        ${LVGL_DIR}/src/model.c)
endif()

# Apps still compiled by `main` use its automatic dependency behavior during
# migration. Independent selected app components declare their own dependencies.
if(MAIN_SRCS)
    idf_component_register(
        SRCS ${MAIN_SRCS}
        INCLUDE_DIRS ${MAIN_INCLUDE_DIRS})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE LVGL_IN_USED=1)
else()
    idf_component_register()
endif()
