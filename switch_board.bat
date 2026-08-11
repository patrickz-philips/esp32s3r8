@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

set "BOARD_COUNT=2"
set "BOARD_1=amoled_175"
set "BOARD_2=amoled_206"
set "BOARD_DESC_1=Waveshare ESP32-S3 Touch AMOLED 1.75\""
set "BOARD_DESC_2=Waveshare ESP32-S3 Touch AMOLED 2.06\""

set "PROJECT_COUNT=4"
set "PROJECT_1=slide_player"
set "PROJECT_2=salary_cat"
set "PROJECT_3=acc_data"
set "PROJECT_4=battery_monitor"
set "PROJ_DESC_1=PNG slideshow (touch gestures)"
set "PROJ_DESC_2=Salary cat (GIF + MP3 from SD)"
set "PROJ_DESC_3=Accelerometer logger (IMU + PMU)"
set "PROJ_DESC_4=Battery / PMU monitor (AXP2101)"

if exist ".board" (
    set /p "cur_board="<".board"
) else (
    set "cur_board=!BOARD_1!"
)

if exist ".lvgl_project" (
    set /p "cur_proj="<".lvgl_project"
) else (
    set "cur_proj=!PROJECT_1!"
)

rem --- Step 1: board -------------------------------------------------------
echo Step 1/2 - Select board (current: !cur_board!)
for /L %%i in (1,1,%BOARD_COUNT%) do (
    set "mark=  "
    if /I "!BOARD_%%i!"=="!cur_board!" set "mark==>"
    echo   !mark! %%i^) !BOARD_%%i! !BOARD_DESC_%%i!
)

set "board=!cur_board!"
:board_prompt
set "c="
set /p "c=  number / Enter=keep / q=quit: "
if /I "!c!"=="q" (
    echo Aborted.
    exit /b 0
)
if "!c!"=="" goto board_done
call :is_number "!c!"
if errorlevel 1 (
    echo   invalid: '!c!'
    goto board_prompt
)
if !c! LSS 1 (
    echo   invalid: '!c!'
    goto board_prompt
)
if !c! GTR %BOARD_COUNT% (
    echo   invalid: '!c!'
    goto board_prompt
)
set "board=!BOARD_%c%!"

:board_done

rem --- Step 2: lvgl project ------------------------------------------------
echo.
echo Step 2/2 - Select lvgl project (current: !cur_proj!)
for /L %%i in (1,1,%PROJECT_COUNT%) do (
    set "mark=  "
    if /I "!PROJECT_%%i!"=="!cur_proj!" set "mark==>"
    echo   !mark! %%i^) !PROJECT_%%i! !PROJ_DESC_%%i!
)

set "project=!cur_proj!"
:project_prompt
set "c="
set /p "c=  number / Enter=keep / q=quit: "
if /I "!c!"=="q" (
    echo Aborted.
    exit /b 0
)
if "!c!"=="" goto project_done
call :is_number "!c!"
if errorlevel 1 (
    echo   invalid: '!c!'
    goto project_prompt
)
if !c! LSS 1 (
    echo   invalid: '!c!'
    goto project_prompt
)
if !c! GTR %PROJECT_COUNT% (
    echo   invalid: '!c!'
    goto project_prompt
)
set "project=!PROJECT_%c%!"

:project_done

>".board" echo(!board!
>".lvgl_project" echo(!project!

rem Each app's entry point lives in main/app_<project>/app_<project>.c
set "app_entry=main\app_!project!\app_!project!.c"
if not exist "!app_entry!" echo   warning: entry point not found: !app_entry!

echo.
echo Selected: board=!board!  project=!project!
echo App entry: !app_entry!
echo (.board / .lvgl_project updated - they are the single source of truth)
echo.
echo Build:           idf.py build
echo Flash ^& monitor: idf.py -p ^<PORT^> flash monitor
echo Or just use the VS Code ESP-IDF "Build" button (shared build/ dir).
exit /b 0

:is_number
set "n=%~1"
echo(%n%| findstr /r "^[0-9][0-9]*$" >nul
if errorlevel 1 exit /b 1
exit /b 0
