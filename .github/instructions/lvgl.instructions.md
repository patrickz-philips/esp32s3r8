---
name: "LVGL Project Changes"
description: "Use when creating or modifying an LVGL project, screen, widget, style, asset, UI event, UI model, LVGL build wiring, or LVGL project selection."
applyTo: "lvgl/**"
---
# LVGL Project Guidelines

- Keep UI implementation changes under `lvgl/<project>/`: screens, widgets,
  styles, assets, UI events, and UI-facing model APIs.
- Use LVGL 9 APIs compatible with the version declared by the project.
- Expose a small public header under `inc/`; keep internal widget details under
  `src/`.
- Keep business state exchange behind the project's model API. FreeRTOS tasks
  and hardware callbacks should post typed data or events instead of mutating
  widgets directly.
- Create and mutate LVGL objects only while the LVGL lock is held or from an
  LVGL-owned callback.
- Do not initialize the BSP, mount storage, configure GPIO, or start sensor,
  audio, or PMU drivers from the LVGL layer.
- For a new LVGL project, update the root supported-project list, selected source
  wiring, and both selector scripts. Pair it with an app component rather than
  placing app startup code in `lvgl/`.
- Application-only dependencies belong to `main/app_<project>/idf_component.yml`.
  Add reusable support as a version-controlled component or Component Manager
  dependency; do not manually populate `managed_components/`.
- Keep non-LVGL edits limited to required build wiring, selector metadata,
  manifests, and reusable support components.
- Build the affected board/app pair and inspect warnings, memory use, and asset
  availability. Hardware rendering still requires a device smoke test.
