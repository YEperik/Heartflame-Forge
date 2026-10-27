#include "../main_window.hpp"
#include <QAbstractItemView>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

QWidget* MainWindow::buildBinaryInspectorPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(12);
    auto* heading = new QLabel(QStringLiteral("Binary Inspector"), page);
    heading->setStyleSheet(QStringLiteral("font-size: 23px; font-weight: 700; color: #233246;"));
    auto* intro = new QLabel(QStringLiteral("Inspeksi read-only. Tidak ada byte yang diubah. Ukuran file dibatasi 64 MiB; hex preview menampilkan maksimum 8 KiB pertama."), page);
    intro->setWordWrap(true);
    intro->setStyleSheet(QStringLiteral("color: #667487;"));
    auto* openButton = new QPushButton(QStringLiteral("Pilih berkas binary…"), page);
    openButton->setMinimumHeight(36);
    connect(openButton, &QPushButton::clicked, this, [this]() { inspectBinaryDialog(); });
    binaryInfo_ = new QLabel(QStringLiteral("Belum ada berkas dipilih."), page);
    binaryInfo_->setWordWrap(true);
    binaryInfo_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    binaryInfo_->setStyleSheet(QStringLiteral("background: white; border: 1px solid #dfe5ed; border-radius: 5px; padding: 10px; color: #2c3949;"));
    auto* searchRow = new QHBoxLayout();
    binarySearch_ = new QLineEdit(page);
    binarySearch_->setPlaceholderText(QStringLiteral("Cari string ASCII di seluruh berkas yang dimuat…"));
    auto* searchButton = new QPushButton(QStringLiteral("Cari"), page);
    connect(searchButton, &QPushButton::clicked, this, [this]() { searchBinaryText(); });
    connect(binarySearch_, &QLineEdit::returnPressed, this, [this]() { searchBinaryText(); });
    searchRow->addWidget(binarySearch_, 1);
    searchRow->addWidget(searchButton);
    hexView_ = new QTextEdit(page);
    hexView_->setObjectName(QStringLiteral("binaryHexPreview"));
    hexView_->setReadOnly(true);
    hexView_->setLineWrapMode(QTextEdit::NoWrap);
    hexView_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    hexView_->setStyleSheet(QStringLiteral("QTextEdit { background: #151a22; color: #d8e3f0; border: 1px solid #2b3544; border-radius: 5px; padding: 8px; }"));
    layout->addWidget(heading);
    layout->addWidget(intro);
    layout->addWidget(openButton, 0, Qt::AlignLeft);
    layout->addWidget(binaryInfo_);
    layout->addLayout(searchRow);
    layout->addWidget(hexView_, 1);
    renderHexPreview();
    return page;
}


