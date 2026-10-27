#include "model_export_page.hpp"

#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStringList>

namespace ps2asset::gui {
ModelExportPage::ModelExportPage(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(14);
    auto* title = new QLabel(QStringLiteral("Model Export — GLB / glTF 2.0"), this);
    title->setStyleSheet(QStringLiteral("font-size: 23px; font-weight: 700; color: #233246;"));
    auto* description = new QLabel(
        QStringLiteral("Assimp dapat mengekspor scene yang berhasil diimpor menjadi GLB. Fitur ini tidak berarti format .mdl dari game sudah didukung: parser geometri game masih diperlukan untuk membangun scene vertex/indeks/material terlebih dahulu."), this);
    description->setWordWrap(true);
    description->setStyleSheet(QStringLiteral("color: #667487; font-size: 13px;"));
    auto* openButton = new QPushButton(QStringLiteral("Pilih input dan ekspor GLB…"), this);
    openButton->setMinimumHeight(36);
    connect(openButton, &QPushButton::clicked, this, [this]() { chooseAndExport(); });
    status_ = new QLabel(this);
    status_->setWordWrap(true);
    status_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    status_->setStyleSheet(QStringLiteral("background: white; border: 1px solid #dfe5ed; border-radius: 6px; padding: 12px; color: #2c3949;"));
#ifndef PS2ASSET_HAS_ASSIMP
    openButton->setEnabled(false);
    status_->setText(QStringLiteral("STATUS: Exporter Assimp tidak tersedia pada build ini. Aktifkan dengan menginstal Assimp development package, lalu konfigurasi ulang CMake. Model game .mdl tetap memerlukan decoder geometri khusus sebelum ekspor.") );
#else
    status_->setText(QStringLiteral("STATUS: Assimp tersedia. Hanya format input yang dikenali Assimp yang bisa diekspor. File sumber tidak akan ditimpa; output GLB harus berupa path baru."));
#endif
    auto* note = new QLabel(QStringLiteral("Yang masih perlu dianalisis untuk .mdl: vertex layout, indeks/triangle strip, orientasi sumbu, UV, material, hierarchy node, skeleton, dan animasi. Tidak ada konversi otomatis dari marker SL01 ke mesh."), this);
    note->setWordWrap(true);
    note->setStyleSheet(QStringLiteral("background: #fff3cd; color: #805b13; border: 1px solid #f1d68a; border-radius: 5px; padding: 10px;"));
    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(openButton, 0, Qt::AlignLeft);
    layout->addWidget(status_);
    layout->addWidget(note);
    layout->addStretch(1);
}

void ModelExportPage::chooseAndExport() {
#ifndef PS2ASSET_HAS_ASSIMP
    QMessageBox::information(this, QStringLiteral("Exporter tidak tersedia"),
                             QStringLiteral("Build ini tidak memiliki Assimp. Instal dependency dan konfigurasi ulang CMake."));
#else
    const QString source = QFileDialog::getOpenFileName(this, QStringLiteral("Pilih model yang didukung Assimp"));
    if (source.isEmpty()) return;
    QString output = QFileDialog::getSaveFileName(this, QStringLiteral("Simpan model GLB"),
        QFileInfo(source).completeBaseName() + QStringLiteral(".glb"), QStringLiteral("glTF Binary (*.glb)"));
    if (output.isEmpty()) return;
    if (!output.endsWith(QStringLiteral(".glb"), Qt::CaseInsensitive)) output += QStringLiteral(".glb");
    if (QFileInfo::exists(output)) {
        QMessageBox::warning(this, QStringLiteral("Output sudah ada"), QStringLiteral("Pilih path output baru. File yang sudah ada tidak ditimpa."));
        return;
    }
#ifdef Q_OS_WIN
    const QString toolName = QStringLiteral("ps2asset-model-export-glb.exe");
#else
    const QString toolName = QStringLiteral("ps2asset-model-export-glb");
#endif
    const QString toolPath = QCoreApplication::applicationDirPath() + QStringLiteral("/") + toolName;
    QProcess process;
    process.start(toolPath, QStringList{source, output});
    if (!process.waitForStarted(5000) || !process.waitForFinished(300000)) {
        process.kill();
        process.waitForFinished(3000);
        QMessageBox::critical(this, QStringLiteral("Ekspor gagal"), QStringLiteral("Exporter tidak dapat dijalankan atau timeout.\n%1").arg(toolPath));
        return;
    }
    const QString details = QString::fromLocal8Bit(process.readAllStandardOutput()) + QString::fromLocal8Bit(process.readAllStandardError());
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        QMessageBox::warning(this, QStringLiteral("Ekspor GLB gagal"), details.isEmpty()
            ? QStringLiteral("Assimp tidak dapat membaca input ini atau menulis GLB.") : details);
        return;
    }
    status_->setText(QStringLiteral("Ekspor selesai: %1\n%2").arg(output, details));
    QMessageBox::information(this, QStringLiteral("Ekspor selesai"), QStringLiteral("GLB berhasil dibuat:\n%1").arg(output));
#endif
}
}  // namespace ps2asset::gui
