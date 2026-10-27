#include "../main_window.hpp"
#include <QAbstractItemView>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

QWidget* MainWindow::buildArchiveBrowserPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(12);
    auto* heading = new QLabel(QStringLiteral("Archive Browser"), page);
    heading->setStyleSheet(QStringLiteral("font-size: 23px; font-weight: 700; color: #233246;"));
    auto* intro = new QLabel(QStringLiteral("Baca metadata ZIP tanpa mengekstrak seluruh arsip. Pilih file TIM2 untuk membuka di Texture Workshop; setelah mengganti tekstur, simpan perubahan sebagai ZIP baru dari menu Tekstur."), page);
    intro->setWordWrap(true);
    intro->setStyleSheet(QStringLiteral("color: #667487;"));
    auto* controls = new QHBoxLayout();
    auto* openButton = new QPushButton(QStringLiteral("Buka arsip ZIP…"), page);
    openButton->setMinimumHeight(36);
    connect(openButton, &QPushButton::clicked, this, [this]() { openArchiveDialog(); });
    archiveFilter_ = new QLineEdit(page);
    archiveFilter_->setPlaceholderText(QStringLiteral("Filter nama/path entri…"));
    connect(archiveFilter_, &QLineEdit::textChanged, this, [this](const QString&) { filterArchiveEntries(); });
    auto* openTextureButton = new QPushButton(QStringLiteral("Buka entri TIM2"), page);
    connect(openTextureButton, &QPushButton::clicked, this, [this]() { openSelectedArchiveTexture(); });
    controls->addWidget(openButton);
    controls->addWidget(archiveFilter_, 1);
    controls->addWidget(openTextureButton);
    archiveSummary_ = new QLabel(QStringLiteral("Belum ada arsip dibuka."), page);
    archiveSummary_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    archiveSummary_->setStyleSheet(QStringLiteral("background: white; border: 1px solid #dfe5ed; border-radius: 5px; padding: 8px; color: #344256;"));

    archiveTable_ = new QTableWidget(page);
    archiveTable_->setObjectName(QStringLiteral("archiveEntryTable"));
    archiveTable_->setColumnCount(3);
    archiveTable_->setHorizontalHeaderLabels(QStringList() << QStringLiteral("Nama / Path") << QStringLiteral("Tipe") << QStringLiteral("Ukuran (byte)"));
    archiveTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    archiveTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    archiveTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    archiveTable_->setAlternatingRowColors(true);
    archiveTable_->setSortingEnabled(false);
    archiveTable_->verticalHeader()->setVisible(false);
    archiveTable_->horizontalHeader()->setStretchLastSection(false);
    archiveTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    archiveTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    archiveTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    connect(archiveTable_, &QTableWidget::cellDoubleClicked, this, [this](int, int) { openSelectedArchiveTexture(); });

    layout->addWidget(heading);
    layout->addWidget(intro);
    layout->addLayout(controls);
    layout->addWidget(archiveSummary_);
    layout->addWidget(archiveTable_, 1);
    return page;
}


