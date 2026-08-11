# Project Guidelines

## Language

- Write all source-code comments, documentation comments, log messages, build
  script comments, and newly added project documentation in English.
- Preserve established C, C++, CMake, shell, and batch formatting nearby.

## Architecture

- Follow the target architecture and migration order in [plan.md](../plan.md).
- Keep `boards/<board>` hardware-specific: BSP, on-board drivers, GPIO and
  capability definitions, partitions, sdkconfig, compatibility metadata, and
  board documentation only.
- Keep reusable, board-independent startup and orchestration directly under
  `main/`. Keep app-specific behavior under `main/app_<name>/`.
- Treat each selected `main/app_<name>` as an ESP-IDF component with its own
  `CMakeLists.txt` and `idf_component.yml`.
- Keep LVGL screens, widgets, styles, assets, and UI model APIs under
  `lvgl/<name>/`. Do not put BSP initialization or hardware policy there.
- Do not make an app component depend on ESP-IDF's special `main` component.
  Move app-consumed reusable services to `components/`, or invert the interface
  so generic `main` calls the selected app component.

## Dependencies

- Board manifests declare only BSP and on-board hardware dependencies.
- App manifests declare app-only libraries such as image or audio decoders.
- Never edit `dependencies.lock` manually.
- Never rely on direct edits under ignored `managed_components/`. Use Component
  Manager, a version-controlled `override_path`, or a maintained fork.
- Avoid relying on `main` automatically seeing every component. Declare public
  and private component dependencies at the owning component boundary.

## Compatibility

- Keep the board/app compatibility source synchronized with top-level CMake and
  both `switch_board.sh` and `switch_board.bat`.
- Reject incompatible board/app pairs during configuration. The selectors must
  display unsupported apps with an `(Incompatible)` suffix after board choice.
- Never infer a GPIO assignment or hardware capability. Verify it from an
  official source or obtain it from the user.

## Validation

- Prefer an isolated build directory named `build/<board>_<app>` when checking
  more than one combination.
- After CMake, manifest, board, or app-selection changes, configure and build at
  least the affected compatible pair.
- Do not report hardware behavior as verified unless it was tested on a device.
- Preserve unrelated user changes and keep edits scoped to the requested layer.
