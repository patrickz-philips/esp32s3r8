---
name: "Build App Logic"
description: "Create or modify generic main logic and app-specific ESP-IDF logic that bridges hardware services to an LVGL model API."
argument-hint: "App name, business behavior, required hardware, and timing/task constraints"
agent: "agent"
---
# Build Application Logic

Follow [project guidelines](../copilot-instructions.md),
[app instructions](../instructions/app-logic.instructions.md), and
[build instructions](../instructions/build-dependencies.instructions.md).

Implement the requested application behavior end to end.

1. Inspect the matching `lvgl/<app>` public UI/model headers, the selected board
   capabilities, and nearby app implementations.
2. Classify each change:
   - Generic startup or coordination belongs directly under `main/`.
   - App-specific lifecycle, tasks, timing, paths, and model adaptation belong
     under `main/app_<app>/`.
   - A service consumed by multiple app components belongs under `components/`
     with a stable API; an app component must not depend on special component
     `main`.
3. Ensure `main/app_<app>` is an independent selected component with its own
   `CMakeLists.txt`, `idf_component.yml`, and English `README.md`.
4. Declare app-only libraries in the app manifest. Keep BSP and on-board device
   dependencies with the board.
5. Connect asynchronous hardware/service data to typed functions in the LVGL
   model API. Do not manipulate private LVGL widgets from app logic.
6. Implement explicit initialization order, error rollback, task ownership,
   stack sizes, priorities, synchronization, and shutdown behavior.
7. Update the app's required capabilities and every board's compatibility entry
   when requirements change. Keep CMake and both selector scripts consistent.
8. Build the affected compatible pair in an isolated directory and verify an
   incompatible pair is rejected when compatibility changed. Separate build
   evidence from hardware test requirements.

In the final response, summarize generic versus app-specific ownership, model
interfaces, dependencies, task behavior, compatibility changes, and validation.
