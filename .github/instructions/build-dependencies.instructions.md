---
name: "Build and Dependency Wiring"
description: "Use when changing CMake, ESP-IDF component manifests, dependencies.lock, sdkconfig layering, app/board selection, or switch_board shell and batch scripts."
applyTo: ["**/CMakeLists.txt", "**/idf_component.yml", "switch_board.sh", "switch_board.bat"]
---
# Build and Dependency Guidelines

- The selected board and selected app are independent inputs, but their
  combination must pass one compatibility check during CMake configuration.
- Register only `boards/${BOARD}` and `main/app_${LVGL_PROJECT}` as selected
  extra component directories. Do not install dependencies for non-selected
  apps or boards.
- Keep dependency ownership aligned with the architecture in `plan.md`.
- Treat `supported_apps.txt` in each board directory as the compatibility source
  consumed by CMake and both selector scripts.
- Keep shell and batch selector behavior equivalent. Show all apps after board
  selection, append `(Incompatible)` where required, and reject unsupported
  choices.
- Keep `.board` and `.lvgl_project` as local generated selection state.
- Do not edit `dependencies.lock` directly; regenerate it through ESP-IDF
  Component Manager after manifest changes and review the resulting diff.
- Do not patch generated `managed_components/` in place. Use a local override or
  pinned fork when upstream behavior must change.
- Use isolated build directories for matrix checks so one pair cannot reuse
  another pair's CMake cache or sdkconfig.
- Validate an affected compatible build and verify that at least one known
  incompatible pair fails at configuration time.
