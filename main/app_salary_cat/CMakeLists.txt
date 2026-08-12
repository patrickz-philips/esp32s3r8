set(SALARY_CAT_UI_DIR "${CMAKE_SOURCE_DIR}/lvgl/salary_cat")

if(NOT DEFINED ENV{SELECTED_BSP_COMPONENT})
	message(FATAL_ERROR "Selected BSP component was not exported by the project")
endif()

idf_component_register(
	SRCS
		"app_salary_cat.c"
		"audio_player.c"
		"${SALARY_CAT_UI_DIR}/src/salary_cat.c"
	INCLUDE_DIRS
		"."
		"${SALARY_CAT_UI_DIR}/inc"
	PRIV_REQUIRES
		chmorgan__esp-libhelix-mp3
		esp_codec_dev
		esp_common
		freertos
		heap
		log
		lvgl__lvgl
		pmu_power
		$ENV{SELECTED_BSP_COMPONENT})

target_compile_definitions(${COMPONENT_LIB} PRIVATE LVGL_IN_USED=1)

if("$ENV{SELECTED_BOARD}" STREQUAL "amoled_175")
	target_compile_definitions(${COMPONENT_LIB} PRIVATE SALARY_CAT_BOARD_AMOLED_175=1)
endif()