---
name: "Modify LVGL Project"
description: "Create or modify an LVGL project while preserving UI, app, board, and dependency boundaries."
argument-hint: "LVGL project name and requested UI/model behavior"
agent: "agent"
---
# Modify an LVGL Project

Follow [project guidelines](../copilot-instructions.md),
[LVGL instructions](../instructions/lvgl.instructions.md), and
[build instructions](../instructions/build-dependencies.instructions.md).

Implement the requested LVGL work end to end.

1. Inspect `lvgl/<project>/inc`, `src`, assets, the matching
   `main/app_<project>` bridge, the current UI/model contract, and the active
   display resolution supplied by the selected board's BSP.
2. Keep screens, widgets, styles, events, assets, and UI model implementation
   inside `lvgl/<project>/`. Preserve a small public API in `inc/`.
3. Default screen and top-level layout dimensions to the current active LVGL
   display's horizontal and vertical pixel resolution. Query them through LVGL
   display APIs; do not hardcode a board resolution or include BSP headers in
   the LVGL layer.
4. Keep hardware initialization, filesystem mounting, FreeRTOS orchestration,
   and board policy out of the LVGL layer. Request those operations through a
   model/app contract.
5. If this is a new LVGL project, update the root supported-project list, build
   wiring, and both selector scripts, and ensure a matching app component owns
   startup logic.
6. Put app-only dependency declarations in
   `main/app_<project>/idf_component.yml`. Add reusable support under
   `components/` or through Component Manager. Never directly create or patch a
   generated managed component.
7. Outside `lvgl/`, edit only required build wiring, selector metadata,
   manifests, app interface declarations, or reusable support components.
8. Reassess board compatibility if the UI adds hardware, memory, storage, or
   decoder requirements.
9. Build the affected board/app pair. Check LVGL API version, locking, active
   display resolution usage, asset paths, compile warnings, and memory impact.
   State which rendering or input behavior still needs hardware validation.

In the final response, identify the UI/model contract, non-LVGL wiring changes,
dependency changes, compatibility impact, and validation results.
