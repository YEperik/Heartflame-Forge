# Workbench extensions

PS2 Asset Workbench discovers optional Qt plugin libraries in:

- `plugins/` beside the `ps2asset-workbench` executable;
- the application data directory reported by Qt's `QStandardPaths::AppDataLocation`, under `plugins/`.

The plugin API currently has IID `org.ps2asset.workbench.IWorkbenchPlugin/1.0`. Implement the interface declared in `include/ps2asset/workbench/plugin_interface.hpp` and expose the implementation with Qt's `Q_PLUGIN_METADATA` and `Q_INTERFACES` macros.

A plugin should implement:

- `id()` — stable, unique identifier;
- `displayName()` — navigation item caption;
- `description()` — tooltip / short explanation;
- `createWidget(QWidget *parent)` — creates its module page.

See `src/gui/plugins/sample_plugin.cpp` for the sample implementation. The CMake build compiles it as `ps2asset_sample_plugin` when Qt Widgets is detected. The desktop application loads plugins dynamically and isolates load failures: invalid or incompatible modules are skipped with a diagnostic rather than preventing the application from starting.

## Compatibility rules

1. Keep public plugin dependencies limited to Qt Widgets and the shared interface header unless extra dependencies are documented.
2. Do not assume unknown game formats. Validate data bounds before parsing user-supplied files.
3. Do not modify or overwrite source assets by default.
4. Keep plugin IDs stable; changing public interface layout requires a new IID version.
5. Ship compatible Qt runtime libraries according to the distribution method chosen by the project.
