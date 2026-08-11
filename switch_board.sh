#!/usr/bin/env bash
# Two-stage selector for the multi-board / multi-app ESP-IDF project.
#   Step 1: choose the target board.
#   Step 2: choose the LVGL application (lvgl/<project> submodule).
# Press 'q' at any prompt to quit, or Enter to keep the current selection.
set -euo pipefail
cd "$(dirname "$0")"

BOARDS=(amoled_175 amoled_206)
BOARD_DESC=(
    "Waveshare ESP32-S3 Touch AMOLED 1.75\""
    "Waveshare ESP32-S3 Touch AMOLED 2.06\""
)
PROJECTS=(slide_player salary_cat acc_data battery_monitor)
PROJ_DESC=(
    "PNG slideshow (touch gestures)"
    "Salary cat (GIF + MP3 from SD)"
    "Accelerometer logger (IMU + PMU)"
    "Battery / PMU monitor (AXP2101)"
)

read_cur() { if [ -f "$1" ]; then tr -d '[:space:]' <"$1"; else printf '%s' "$2"; fi; }
cur_board="$(read_cur .board "${BOARDS[0]}")"
cur_proj="$(read_cur .lvgl_project "${PROJECTS[0]}")"

# --- Step 1: board ----------------------------------------------------------
echo "Step 1/2 - Select board (current: $cur_board)"
for i in "${!BOARDS[@]}"; do
    mark="  "; [ "${BOARDS[$i]}" = "$cur_board" ] && mark="=>"
    printf "  %s %d) %-12s %s\n" "$mark" "$((i + 1))" "${BOARDS[$i]}" "${BOARD_DESC[$i]}"
done
board="$cur_board"
while true; do
    printf "  number / Enter=keep / q=quit: "
    read -r c || c="q"
    case "$c" in
        q | Q) echo "Aborted."; exit 0 ;;
        "") break ;;
        *)
            if [ "$c" -eq "$c" ] 2>/dev/null && [ "$c" -ge 1 ] && [ "$c" -le "${#BOARDS[@]}" ]; then
                board="${BOARDS[$((c - 1))]}"; break
            fi
            echo "  invalid: '$c'" ;;
    esac
done

# --- Step 2: lvgl project ---------------------------------------------------
echo
echo "Step 2/2 - Select lvgl project (current: $cur_proj)"
for i in "${!PROJECTS[@]}"; do
    mark="  "; [ "${PROJECTS[$i]}" = "$cur_proj" ] && mark="=>"
    printf "  %s %d) %-16s %s\n" "$mark" "$((i + 1))" "${PROJECTS[$i]}" "${PROJ_DESC[$i]}"
done
project="$cur_proj"
while true; do
    printf "  number / Enter=keep / q=quit: "
    read -r c || c="q"
    case "$c" in
        q | Q) echo "Aborted."; exit 0 ;;
        "") break ;;
        *)
            if [ "$c" -eq "$c" ] 2>/dev/null && [ "$c" -ge 1 ] && [ "$c" -le "${#PROJECTS[@]}" ]; then
                project="${PROJECTS[$((c - 1))]}"; break
            fi
            echo "  invalid: '$c'" ;;
    esac
done

printf '%s\n' "$board" >.board
printf '%s\n' "$project" >.lvgl_project

# Each app's entry point lives in main/app_<project>/app_<project>.c
app_entry="main/app_${project}/app_${project}.c"
[ -f "$app_entry" ] || echo "  warning: entry point not found: $app_entry"

echo
echo "Selected: board=$board  project=$project"
echo "App entry: $app_entry"
echo "(.board / .lvgl_project updated - they are the single source of truth)"
echo
echo "Build:           idf.py build"
echo "Flash & monitor: idf.py -p <PORT> flash monitor"
echo "Or just use the VS Code ESP-IDF 'Build' button (shared build/ dir)."
