---
name: "Application Logic"
description: "Use when creating or modifying app business logic, app_main, FreeRTOS tasks, reusable main services, hardware-to-model adapters, or files under main/app_<name>."
applyTo: "main/**"
---
# Application Logic Guidelines

- Keep files directly under `main/` generic and board-independent. Move fixed
  asset paths, app-specific timing, one-app tasks, and app-only policies into
  `main/app_<name>/`.
- Give every app directory its own `CMakeLists.txt`, `idf_component.yml`, and
  English `README.md` describing behavior, required capabilities, dependencies,
  task model, and LVGL model contract.
- Declare image, audio, protocol, and other app-only dependencies in the app
  manifest, not a board manifest.
- Do not make an app component depend on the special `main` component. If more
  than one app consumes a service, move it to `components/<service>` with a
  stable public API. Otherwise keep it in the owning app.
- Keep BSP and driver initialization in app/common logic, not in LVGL UI code.
  Check board capabilities before compiling or selecting an app.
- Bridge hardware and asynchronous work to `lvgl/<name>` through its typed model
  interface. Do not reach into private widget state from app code.
- Define initialization order, rollback on partial failure, task stack sizes,
  priorities, core affinity only when needed, and shutdown ownership.
- Check all ESP-IDF return values. Do not continue into UI or tasks after a
  required subsystem fails.
- Update compatibility metadata whenever an app's required hardware changes.
- Build the affected pair and separate compile-time validation from on-device
  checks for display, touch, storage, sensor, audio, and PMU behavior.
