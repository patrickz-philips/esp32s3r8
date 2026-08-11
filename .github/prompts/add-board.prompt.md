---
name: "Add ESP32 Board"
description: "Add a commercial or custom ESP32 board, document its hardware, wire its BSP, and calculate board/app compatibility."
argument-hint: "Board ID plus either the exact board model or custom SoC/GPIO/sensor specification"
agent: "agent"
---
# Add a Board

Follow [project guidelines](../copilot-instructions.md),
[board instructions](../instructions/boards.instructions.md), and
[build instructions](../instructions/build-dependencies.instructions.md).

Add the requested board end to end. Do not stop after proposing files.

1. Determine the input mode:
   - Named board: research official product documentation, schematic, vendor BSP,
     and ESP Component Registry entries. Prefer primary sources and record exact
     component versions or constraints.
   - Custom board: use the user's SoC, GPIO allocation, buses, display, touch,
     storage, sensors, audio, PMU, flash, and PSRAM specification. Ask concise
     questions for missing facts that would make an implementation unsafe.
2. Inspect the closest existing board and the current build-selection flow.
3. Create `boards/<id>/CMakeLists.txt`, `idf_component.yml`, `board_config.h`,
   `supported_apps.txt`, `sdkconfig.board`, `partitions.csv`, and an English
   `README.md`.
   The README must include this populated table, with one row for every occupied
   ESP GPIO:

   | GPIO | Signal/Function | Peripheral/Sensor | Bus | Direction | Shared/Conflict | Source |
   |------|-----------------|-------------------|-----|-----------|-----------------|--------|

   Link each GPIO to its sensor or peripheral. Mark shared, multiplexed,
   boot-strapping, input-only, and otherwise constrained pins. Use `N/A` where a
   field does not apply, and cite an official schematic, BSP symbol/file,
   datasheet, or explicit user specification in every `Source` cell. Also add a
   sensor/peripheral summary for devices without a direct ESP GPIO, including
   bus, address or chip select, interrupt/reset signals, and related GPIO rows.
4. Keep only BSP and on-board hardware dependencies in the board manifest. Do
   not copy vendor driver code when a suitable maintained component exists.
5. Audit every existing app's hardware and API requirements. Put only compatible
   app IDs in `supported_apps.txt` and explain exclusions in the board README.
6. Update root CMake, `switch_board.sh`, and `switch_board.bat`. After board
   selection, both selectors must list all apps, label unsupported ones with
   `(Incompatible)`, and prevent their selection. CMake must reject the same
   unsupported pair even when variables are supplied directly.
7. Resolve component dependencies without manually editing
   `managed_components/` or `dependencies.lock`.
8. Configure and build at least one compatible pair in an isolated build
   directory. Verify partition and binary sizes. Report unperformed device smoke
   tests explicitly.

In the final response, summarize verified hardware facts and sources, files
changed, compatibility decisions, build results, and remaining device checks.
