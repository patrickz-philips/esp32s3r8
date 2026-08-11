---
name: "Validate Board App Matrix"
description: "Audit and validate all declared board/app compatibility combinations, dependency ownership, and selector behavior."
argument-hint: "Optional board or app subset; default is the full matrix"
agent: "agent"
---
# Validate the Board and App Matrix

Follow [project guidelines](../copilot-instructions.md) and
[build instructions](../instructions/build-dependencies.instructions.md).

Audit the requested subset or the full board/app matrix.

1. Discover boards, apps, `supported_apps.txt` files, capability flags, app
   requirements, root CMake lists, and both selector implementations.
2. Report disagreements between documentation, capability flags, compatibility
   files, CMake validation, and selector labels before editing.
3. Fix metadata or wiring only when the intended compatibility can be proven
   from hardware and app requirements. Do not guess GPIOs or device support.
4. Check dependency ownership: hardware in board manifests, app libraries in app
   manifests, shared services in reusable components, and no durable patches in
   generated managed components.
5. Configure and build each declared compatible pair in a separate
   `build/<board>_<app>` directory. For a large matrix, ask before flashing, but
   do not skip compile validation.
6. Confirm representative incompatible pairs fail during CMake configuration
   and appear with `(Incompatible)` in both selectors.
7. Report results as a compact matrix with configure/build status, binary size,
   skipped hardware tests, and actionable failures.
