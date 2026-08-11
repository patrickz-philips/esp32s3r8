---
name: "Board Integration"
description: "Use when adding, importing, documenting, or changing an ESP32 board, BSP, GPIO map, sensor, board capability, partition table, sdkconfig, or board/app compatibility."
applyTo: "boards/**"
---
# Board Integration Guidelines

- Start from the closest existing board directory, but verify every hardware
  fact independently.
- Support two input modes:
  1. For a named commercial board, research its official product documentation,
     schematic, BSP repository, and ESP Component Registry package.
  2. For a custom board, use the SoC, occupied GPIOs, buses, sensors, display,
     touch, storage, audio, PMU, flash, and PSRAM data supplied by the user. Ask
     for missing critical facts and never invent pin assignments.
- A board directory must contain `CMakeLists.txt`, `idf_component.yml`,
  `board_config.h`, `supported_apps.txt`, `sdkconfig.board`, `partitions.csv`,
  and `README.md`.
- Keep the board component configuration-only unless a small board adapter is
  genuinely needed. Prefer the vendor BSP over copied driver implementations.
- Declare BSP and on-board hardware dependencies only. Do not add PNG, GIF, MP3,
  or other app-only dependencies to a board manifest.
- Document in English: board and SoC identity, flash/PSRAM, display and touch,
  GPIO table, buses, sensors, storage, audio, PMU, dependency versions,
  partition layout, supported apps, initialization notes, and known limits.
- The board README must contain this GPIO allocation table:

  | GPIO | Signal/Function | Peripheral/Sensor | Bus | Direction | Shared/Conflict | Source |
  |------|-----------------|-------------------|-----|-----------|-----------------|--------|

  Add one row for every occupied ESP GPIO. Associate the signal with its sensor
  or peripheral, including display, touch, SD, audio, PMU, buttons, interrupts,
  reset lines, and expansion connectors. Mark shared, multiplexed,
  boot-strapping, input-only, or otherwise constrained pins. Use `N/A` for a
  field that does not apply; do not silently omit it. Cite an official schematic,
  BSP symbol/file, datasheet, or explicit user specification in `Source`.
- Add a sensor/peripheral summary for devices without a direct ESP GPIO. Record
  their bus, I2C address or chip select, interrupt/reset signals when applicable,
  and references to the related GPIO table rows.
- Audit every existing app against the board capabilities. Record compatible
  apps in `supported_apps.txt`, one app ID per line.
- Update root board registration and both selector scripts. After board
  selection, list every app, mark unsupported entries `(Incompatible)`, and
  prevent accidental selection. CMake must independently reject the same pair.
- Validate at least one compatible clean build, partition layout, and binary
  size. Describe required on-device smoke tests separately.
