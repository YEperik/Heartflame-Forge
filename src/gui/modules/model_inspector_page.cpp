#include "../main_window.hpp"
#include <QFontDatabase>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

QWidget* MainWindow::buildModelInspectorPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(12);

    auto* heading = new QLabel(QStringLiteral("Model Inspector — MDL / SL01"), page);
    heading->setStyleSheet(QStringLiteral("font-size: 23px; font-weight: 700; color: #233246;"));
    auto* intro = new QLabel(
        QStringLiteral("Profil read-only khusus model .mdl. Memeriksa pola header, invariant SL01, marker empat-byte, dan kandidat path. Geometri belum didekode; gunakan Animation Inspector untuk riset terpisah. Tidak ada file yang diubah."),
        page);
    intro->setWordWrap(true);
    intro->setStyleSheet(QStringLiteral("color: #667487;"));

    auto* openButton = new QPushButton(QStringLiteral("Buka berkas model .mdl…"), page);
    openButton->setMinimumHeight(36);
    connect(openButton, &QPushButton::clicked, this, [this]() { inspectModelDialog(); });

    modelReport_ = new QPlainTextEdit(page);
    modelReport_->setObjectName(QStringLiteral("modelInspectionReport"));
    modelReport_->setReadOnly(true);
    modelReport_->setLineWrapMode(QPlainTextEdit::NoWrap);
    modelReport_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    modelReport_->setPlaceholderText(
        QStringLiteral("Pilih berkas .mdl untuk melihat laporan byte-level. Ukuran maksimum 64 MiB."));
    modelReport_->setStyleSheet(QStringLiteral(
        "QPlainTextEdit { background: #151a22; color: #d8e3f0; border: 1px solid #2a3442; padding: 10px; }"));

    layout->addWidget(heading);
    layout->addWidget(intro);
    layout->addWidget(openButton, 0, Qt::AlignLeft);
    layout->addWidget(modelReport_, 1);
    return page;
}


