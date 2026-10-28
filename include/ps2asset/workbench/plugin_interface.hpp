#pragma once

#include <QString>
#include <QWidget>
#include <QtPlugin>

namespace ps2asset::workbench {

// Stable, deliberately small boundary between the Workbench shell and optional
// Qt plugin modules. The interface version is part of the plugin IID below.
class IWorkbenchPlugin {
public:
    virtual ~IWorkbenchPlugin() = default;

    [[nodiscard]] virtual QString id() const = 0;
    [[nodiscard]] virtual QString displayName() const = 0;
    [[nodiscard]] virtual QString description() const = 0;
    [[nodiscard]] virtual QWidget* createWidget(QWidget* parent) = 0;
};

}  // namespace ps2asset::workbench

#define PS2ASSET_WORKBENCH_PLUGIN_IID "org.ps2asset.workbench.IWorkbenchPlugin/1.0"
Q_DECLARE_INTERFACE(ps2asset::workbench::IWorkbenchPlugin, PS2ASSET_WORKBENCH_PLUGIN_IID)
