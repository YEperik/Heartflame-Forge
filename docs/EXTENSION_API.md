# Extension API v1.0

Public header: `include/ps2asset/workbench/plugin_interface.hpp`.

```cpp
class IWorkbenchPlugin {
public:
    virtual ~IWorkbenchPlugin() = default;
    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual QString description() const = 0;
    virtual QWidget* createWidget(QWidget* parent) = 0;
};
```

The interface IID is intentionally retained as `org.ps2asset.workbench.IWorkbenchPlugin/1.0` for plugin/source compatibility with the initial workbench series, even though the product has been rebranded to Heartflame Forge. The legacy IID is an ABI identifier, not the application name. See `src/gui/plugins/sample_plugin.cpp` for the required Qt metadata and interface macros.

## Build model

A plugin is a Qt dynamic library (`.dll`, `.so`, or `.dylib` depending on platform) built against a compatible Qt major version and C++ runtime. Put the library into `plugins/` next to `HeartflameForge.exe` or into the application-data `plugins` directory.

## Host behavior

- Plugin IDs must be nonempty and unique.
- A plugin must return a nonempty display name and a non-null widget.
- Incompatible libraries are skipped; the host reports diagnostic messages instead of aborting startup.
- Plugin-created widgets become stack pages in the main window.
- Plugin modules should not mutate source files without an explicit user action and should validate all untrusted binary offsets and sizes.

Because plugins execute native code in-process, only load extensions you trust. API-breaking changes require a new IID.


## Built-in feature modules (v1.1.0)

The built-in workspace catalog is exposed through `ps2asset/workbench/module_catalog.hpp`. Each item carries a stable ID, category, summary, readiness enum, supported input labels, implemented capabilities, and missing-analysis list. The GUI uses the catalog for sidebar display names and status tooltips. This catalog is separate from the Qt plugin ABI so the core can build without Qt.

For a new built-in module, add its parser/API under `include/ps2asset/<domain>/` + `src/<domain>/`, add tests and optional CLI, add a `ModuleDescriptor`, create a page class in `src/gui/modules/`, and register its page in the main stack in the same order as the catalog. External extensions should continue to implement `IWorkbenchPlugin`; they should not depend on private classes in `MainWindow`.
