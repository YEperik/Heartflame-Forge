#pragma once

#include "ps2asset/workbench/plugin_interface.hpp"
#include <QPluginLoader>
#include <QStringList>
#include <QWidget>
#include <memory>
#include <vector>

class QPluginLoader;

namespace ps2asset::workbench {

struct LoadedPluginModule {
    QString id;
    QString name;
    QString description;
    QWidget* widget = nullptr;
    QString filePath;
};

class PluginManager final {
public:
    PluginManager() = default;
    ~PluginManager();
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    // Reads only plugin libraries from the supplied folders. Invalid libraries
    // are skipped and explained in errors(); one bad extension cannot stop startup.
    void discover(const QStringList& directories, QWidget* widgetParent);

    [[nodiscard]] const std::vector<LoadedPluginModule>& modules() const noexcept { return modules_; }
    [[nodiscard]] const QStringList& errors() const noexcept { return errors_; }

private:
    std::vector<std::unique_ptr<QPluginLoader>> loaders_;
    std::vector<LoadedPluginModule> modules_;
    QStringList errors_;
};

}  // namespace ps2asset::workbench
