#include "plugin_manager.hpp"

#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QPluginLoader>
#include <QObject>
#include <QSet>
#include <algorithm>
#include <exception>

namespace ps2asset::workbench {

PluginManager::~PluginManager() = default;

namespace {
bool isLibraryCandidate(const QFileInfo& info) {
#ifdef Q_OS_WIN
    return info.suffix().compare(QStringLiteral("dll"), Qt::CaseInsensitive) == 0;
#elif defined(Q_OS_MACOS)
    const auto suffix = info.suffix().toLower();
    return suffix == QStringLiteral("dylib") || suffix == QStringLiteral("so") || info.fileName().endsWith(QStringLiteral(".bundle"));
#else
    return info.fileName().contains(QStringLiteral(".so"));
#endif
}
}  // namespace

void PluginManager::discover(const QStringList& directories, QWidget* widgetParent) {
    QSet<QString> knownIds;
    for (const auto& module : modules_) knownIds.insert(module.id);

    for (const QString& directory : directories) {
        QDir folder(directory);
        if (!folder.exists()) continue;
        const auto files = folder.entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
        for (const QFileInfo& fileInfo : files) {
            if (!isLibraryCandidate(fileInfo)) continue;

            auto loader = std::make_unique<QPluginLoader>(fileInfo.absoluteFilePath());
            loader->setLoadHints(QLibrary::PreventUnloadHint);
            QObject* instance = loader->instance();
            if (instance == nullptr) {
                errors_.push_back(QStringLiteral("%1: %2").arg(fileInfo.fileName(), loader->errorString()));
                continue;
            }
            auto* plugin = qobject_cast<IWorkbenchPlugin*>(instance);
            if (plugin == nullptr) {
                errors_.push_back(QStringLiteral("%1: tidak mengimplementasikan IWorkbenchPlugin v1.0").arg(fileInfo.fileName()));
                continue;
            }

            const QString moduleId = plugin->id().trimmed();
            const QString moduleName = plugin->displayName().trimmed();
            if (moduleId.isEmpty() || moduleName.isEmpty()) {
                errors_.push_back(QStringLiteral("%1: ID modul atau nama tampilan kosong").arg(fileInfo.fileName()));
                continue;
            }
            if (knownIds.contains(moduleId)) {
                errors_.push_back(QStringLiteral("%1: ID plugin duplikat '%2'").arg(fileInfo.fileName(), moduleId));
                continue;
            }

            // Keep plugin code resident while any plugin-created QWidget exists.
            loader->setLoadHints(QLibrary::PreventUnloadHint);
            try {
                QWidget* widget = plugin->createWidget(widgetParent);
                if (widget == nullptr) {
                    errors_.push_back(QStringLiteral("%1: createWidget() menghasilkan null").arg(fileInfo.fileName()));
                    continue;
                }
                modules_.push_back(LoadedPluginModule{moduleId, moduleName, plugin->description(), widget,
                                                       fileInfo.absoluteFilePath()});
                knownIds.insert(moduleId);
                loaders_.push_back(std::move(loader));
            } catch (const std::exception& ex) {
                errors_.push_back(QStringLiteral("%1: createWidget gagal: %2")
                                       .arg(fileInfo.fileName(), QString::fromUtf8(ex.what())));
            } catch (...) {
                errors_.push_back(QStringLiteral("%1: createWidget gagal dengan exception tak dikenal").arg(fileInfo.fileName()));
            }
        }
    }
}

}  // namespace ps2asset::workbench
