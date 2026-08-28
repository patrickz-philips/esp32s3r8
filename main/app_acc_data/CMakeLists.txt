set(ACC_DATA_UI_DIR "${CMAKE_SOURCE_DIR}/lvgl/acc_data")

if(NOT DEFINED ENV{SELECTED_BOARD})
	message(FATAL_ERROR "Selected board component was not exported by the project")
endif()
if(NOT DEFINED ENV{SELECTED_BSP_COMPONENT})
	message(FATAL_ERROR "Selected BSP component was not exported by the project")
endif()

set(ACC_DATA_SRCS
	"app_acc_data.c"
	"imu.c"
	"${ACC_DATA_UI_DIR}/src/acc_data.c"
	"${ACC_DATA_UI_DIR}/src/model.c")
set(ACC_DATA_PRIV_REQUIRES
	esp_common
	esp_driver_gpio
	freertos
	log
	lvgl__lvgl
	pmu_power
	waveshare__qmi8658
	$ENV{SELECTED_BOARD}
	$ENV{SELECTED_BSP_COMPONENT})

# The SD accelerometer logger only builds on boards with micro-SD storage.
# BOARD_HAS_SD is exported by the top-level CMakeLists.txt from board_config.h;
# boards without it (e.g. touch_lcd_1_28) drop the writer source and the sdmmc
# dependency, and app_acc_data.c/model.c guard the call sites.
if(NOT "$ENV{BOARD_HAS_SD}" STREQUAL "0")
	list(APPEND ACC_DATA_SRCS "sd_acc_writer.c")
	list(APPEND ACC_DATA_PRIV_REQUIRES sdmmc)
endif()

idf_component_register(
	SRCS ${ACC_DATA_SRCS}
	INCLUDE_DIRS
		"."
		"${ACC_DATA_UI_DIR}/inc"
	PRIV_REQUIRES ${ACC_DATA_PRIV_REQUIRES})

target_compile_definitions(${COMPONENT_LIB} PRIVATE LVGL_IN_USED=1)

if("$ENV{SELECTED_BOARD}" STREQUAL "amoled_175")
	target_compile_definitions(${COMPONENT_LIB} PRIVATE ACC_DATA_BOARD_AMOLED_175=1)
elseif("$ENV{SELECTED_BOARD}" STREQUAL "amoled_206")
	target_compile_definitions(${COMPONENT_LIB} PRIVATE ACC_DATA_BOARD_AMOLED_206=1)
endif()