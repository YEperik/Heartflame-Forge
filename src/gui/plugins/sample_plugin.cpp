#include "ps2asset/workbench/plugin_interface.hpp"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

class SampleDiagnosticsPlugin final : public QObject, public ps2asset::workbench::IWorkbenchPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PS2ASSET_WORKBENCH_PLUGIN_IID)
    Q_INTERFACES(ps2asset::workbench::IWorkbenchPlugin)

public:
    QString id() const override { return QStringLiteral("sample.diagnostics"); }
    QString displayName() const override { return QStringLiteral("Contoh Extension"); }
    QString description() const override {
        return QStringLiteral("Contoh modul dinamis untuk memvalidasi extension API Heartflame Forge.");
    }

    QWidget* createWidget(QWidget* parent) override {
        auto* page = new QWidget(parent);
        auto* layout = new QVBoxLayout(page);
        auto* title = new QLabel(QStringLiteral("Extension API berhasil dimuat"), page);
        title->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: 600;"));
        auto* description = new QLabel(
            QStringLiteral("Halaman ini disediakan oleh plugin eksternal. Handler format di masa depan dapat menambahkan panel, exporter, validator, atau alat analisis tanpa mengubah shell utama.\n\nID: sample.diagnostics\nAPI: IWorkbenchPlugin/1.0"), page);
        description->setWordWrap(true);
        description->setTextInteractionFlags(Qt::TextSelectableByMouse);
        layout->addWidget(title);
        layout->addWidget(description);
        layout->addStretch(1);
        return page;
    }
};

#include "sample_plugin.moc"
