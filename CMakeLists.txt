# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# --- Multi-board / multi-app selection --------------------------------------
# The persisted files written by switch_board.sh are the source of truth:
#   priority = BOARD/LVGL_PROJECT environment variable > persisted file > default.
# The value is force-written to the cache on every configure, so switching takes
# effect even in a shared build directory (self-heals a stale cache). The files
# are configure dependencies, so editing them triggers a reconfigure.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/.board"
    "${CMAKE_CURRENT_LIST_DIR}/.lvgl_project")

# resolve_selection(<VAR> <file> <default>)
function(resolve_selection var file default)
    if(DEFINED ENV{${var}})
        set(want "$ENV{${var}}")
    elseif(EXISTS "${file}")
        file(READ "${file}" want)
        string(STRIP "${want}" want)
    else()
        set(want "${default}")
    endif()
    set(${var} "${want}" CACHE STRING "" FORCE)
endfunction()

set(SUPPORTED_BOARDS amoled_175 amoled_206)
set(SUPPORTED_PROJECTS slide_player acc_data battery_monitor)

resolve_selection(BOARD "${CMAKE_CURRENT_LIST_DIR}/.board" amoled_175)
resolve_selection(LVGL_PROJECT "${CMAKE_CURRENT_LIST_DIR}/.lvgl_project" slide_player)

if(NOT BOARD IN_LIST SUPPORTED_BOARDS)
    message(FATAL_ERROR "Unknown BOARD='${BOARD}'. Supported boards: ${SUPPORTED_BOARDS}")
endif()
if(NOT LVGL_PROJECT IN_LIST SUPPORTED_PROJECTS)
    message(FATAL_ERROR "Unknown LVGL_PROJECT='${LVGL_PROJECT}'. Supported: ${SUPPORTED_PROJECTS}")
endif()

message(STATUS "Building for BOARD=${BOARD}")
message(STATUS "Building LVGL_PROJECT=${LVGL_PROJECT}")
# ----------------------------------------------------------------------------

set(BOARD_DIR "${CMAKE_CURRENT_LIST_DIR}/boards/${BOARD}")

# Only the selected board directory is registered, so the component manager
# installs just this board's managed dependencies.
set(EXTRA_COMPONENT_DIRS "${BOARD_DIR}")

# Per-board generated config + layered defaults (board file overrides common).
set(SDKCONFIG "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.${BOARD}")
set(SDKCONFIG_DEFAULTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults;${BOARD_DIR}/sdkconfig.board")
# ----------------------------------------------------------------------------

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

add_compile_options(-Wno-format)
project(app_${BOARD}_${LVGL_PROJECT})
