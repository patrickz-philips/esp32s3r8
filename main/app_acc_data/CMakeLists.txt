set(ACC_DATA_UI_DIR "${CMAKE_SOURCE_DIR}/lvgl/acc_data")

if(NOT DEFINED ENV{SELECTED_BOARD})
	message(FATAL_ERROR "Selected board component was not exported by the project")
endif()
if(NOT DEFINED ENV{SELECTED_BSP_COMPONENT})
	message(FATAL_ERROR "Selected BSP component was not exported by the project")
endif()

idf_component_register(
	SRCS
		"app_acc_data.c"
		"imu.c"
		"sd_acc_writer.c"
		"${ACC_DATA_UI_DIR}/src/acc_data.c"
		"${ACC_DATA_UI_DIR}/src/model.c"
	INCLUDE_DIRS
		"."
		"${ACC_DATA_UI_DIR}/inc"
	PRIV_REQUIRES
		esp_common
		esp_driver_gpio
		freertos
		log
		lvgl__lvgl
		pmu_power
		sdmmc
		waveshare__qmi8658
		$ENV{SELECTED_BOARD}
		$ENV{SELECTED_BSP_COMPONENT})

target_compile_definitions(${COMPONENT_LIB} PRIVATE LVGL_IN_USED=1)

if("$ENV{SELECTED_BOARD}" STREQUAL "amoled_175")
	target_compile_definitions(${COMPONENT_LIB} PRIVATE ACC_DATA_BOARD_AMOLED_175=1)
elseif("$ENV{SELECTED_BOARD}" STREQUAL "amoled_206")
	target_compile_definitions(${COMPONENT_LIB} PRIVATE ACC_DATA_BOARD_AMOLED_206=1)
endif()