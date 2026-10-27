#include "main_window.hpp"

#include <QApplication>
#include <QAction>
#include <QListWidget>
#include <QStackedWidget>
#include <QStyleFactory>
#include <QDebug>
#include <QListWidgetItem>
#include <QPlainTextEdit>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("PS2 Asset Workbench");
    QApplication::setOrganizationName("PS2AssetWorkbench");
    QApplication::setApplicationVersion("1.0.0");

    if (QStyle* style = QStyleFactory::create("Fusion")) {
        QApplication::setStyle(style);
    }

    MainWindow window;
    if (app.arguments().contains(QStringLiteral("--smoke-test"))) {
        const auto* navigation = window.findChild<QListWidget*>(QStringLiteral("moduleNavigation"));
        const auto* stack = window.findChild<QStackedWidget*>(QStringLiteral("moduleStack"));
        if (navigation == nullptr || stack == nullptr || navigation->count() != stack->count() || navigation->count() < 7) {
            qCritical("GUI startup smoke test failed: module navigation and page stack are inconsistent.");
            return 2;
        }
        const auto* saveZipAction = window.findChild<QAction*>(QStringLiteral("saveZipAction"));
        if (saveZipAction == nullptr || saveZipAction->isEnabled()) {
            qCritical("GUI startup smoke test failed: save-to-new-ZIP action missing or incorrectly enabled without a selected edit.");
            return 4;
        }
        const auto* modelReport = window.findChild<QPlainTextEdit*>(QStringLiteral("modelInspectionReport"));
        if (modelReport == nullptr || !modelReport->isReadOnly()) {
            qCritical("GUI startup smoke test failed: read-only model report panel missing.");
            return 5;
        }
#ifdef PS2ASSET_EXPECT_SAMPLE_PLUGIN
        bool foundSamplePlugin = false;
        for (int i = 0; i < navigation->count(); ++i) {
            if (navigation->item(i)->text() == QStringLiteral("Contoh Extension")) {
                foundSamplePlugin = true;
                break;
            }
        }
        if (!foundSamplePlugin) {
            qCritical("GUI startup smoke test failed: sample extension was not loaded.");
            return 3;
        }
#endif
        qInfo("GUI startup smoke test passed: %d module pages loaded.", navigation->count());
        return 0;
    }

    window.show();
    return app.exec();
}
