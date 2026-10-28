# Heartflame Forge extensions

Heartflame Forge discovers optional Qt plugin libraries in:

- `plugins/` beside the `HeartflameForge` executable;
- the application data directory reported by Qt's `QStandardPaths::AppDataLocation`, under `plugins/`.

The current plugin IID is intentionally kept as `org.ps2asset.workbench.IWorkbenchPlugin/1.0` for source/plugin ABI continuity with the initial workbench series. This legacy identifier is not the product name. Implement the interface declared in `include/ps2asset/workbench/plugin_interface.hpp` and expose the implementation with Qt's `Q_PLUGIN_METADATA` and `Q_INTERFACES` macros.

A plugin should implement:

- `id()` — stable, unique identifier;
- `displayName()` — navigation item caption;
- `description()` — tooltip / short explanation;
- `createWidget(QWidget *parent)` — creates its module page.

See `src/gui/plugins/sample_plugin.cpp` for the sample implementation. CMake builds it as `heartflame_sample_plugin` when Qt Widgets and `PS2ASSET_BUILD_SAMPLE_PLUGIN=ON` are available. The desktop application loads plugins dynamically and isolates load failures: invalid or incompatible modules are skipped with a diagnostic rather than preventing the application from starting.

## Compatibility and safety

1. Keep public plugin dependencies limited to Qt Widgets and the interface header unless extra dependencies are documented.
2. Do not assume unknown game formats. Validate data bounds before parsing user-supplied files.
3. Do not modify source assets without explicit user action; default to a new output path.
4. Keep plugin IDs stable. A breaking interface change must use a new IID and migration note.
5. A native plugin DLL is not sandboxed. Review source, origin and dependencies before loading it; plugin code runs with the current user's permissions.
6. Ship compatible Qt runtime libraries and their notices according to the selected distribution method.
