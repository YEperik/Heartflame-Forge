#include "main_window.hpp"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("PS2 Asset Workbench");
    QApplication::setOrganizationName("PS2AssetWorkbench");
    QApplication::setApplicationVersion("0.2.0");

    if (QStyle* style = QStyleFactory::create("Fusion")) {
        QApplication::setStyle(style);
    }

    MainWindow window;
    window.show();
    return app.exec();
}
