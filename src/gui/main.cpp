#include "main_window.hpp"

#include <QApplication>
#include <QListWidget>
#include <QStackedWidget>
#include <QStyleFactory>
#include <QDebug>
#include <QListWidgetItem>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("PS2 Asset Workbench");
    QApplication::setOrganizationName("PS2AssetWorkbench");
    QApplication::setApplicationVersion("0.3.0");

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
