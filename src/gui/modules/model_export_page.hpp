#pragma once
#include <QWidget>
class QLabel;
namespace ps2asset::gui {
class ModelExportPage final : public QWidget {
public:
    explicit ModelExportPage(QWidget* parent = nullptr);
private:
    QLabel* status_ = nullptr;
    void chooseAndExport();
};
}
