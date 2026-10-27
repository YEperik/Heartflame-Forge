#include "../main_window.hpp"
#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSplitter>
#include <QVBoxLayout>

QWidget* MainWindow::buildTexturePage() {
    auto* root = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(18, 16, 18, 14);
    rootLayout->setSpacing(10);

    auto* heading = new QLabel(QStringLiteral("Texture Workshop"), root);
    heading->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: 700; color: #233246;"));
    auto* subtitle = new QLabel(QStringLiteral("TIM2 texture preview, PNG export, image replacement, and safe save-as workflow"), root);
    subtitle->setStyleSheet(QStringLiteral("color: #667487;"));
    rootLayout->addWidget(heading);
    rootLayout->addWidget(subtitle);

    sourceLabel_ = new QLabel(QStringLiteral("Sumber: belum ada file dibuka"), root);
    sourceLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    sourceLabel_->setStyleSheet(QStringLiteral("color: #4c5968; background: white; border: 1px solid #dfe5ed; border-radius: 5px; padding: 8px;"));
    rootLayout->addWidget(sourceLabel_);

    auto* splitter = new QSplitter(Qt::Horizontal, root);
    textureList_ = new QListWidget(splitter);
    textureList_->setObjectName(QStringLiteral("textureList"));
    textureList_->setMinimumWidth(280);
    textureList_->setMaximumWidth(470);
    textureList_->setSelectionMode(QAbstractItemView::SingleSelection);
    textureList_->setUniformItemSizes(false);
    textureList_->setAlternatingRowColors(true);
    textureList_->setStyleSheet(QStringLiteral("QListWidget { background: white; border: 1px solid #dfe5ed; border-radius: 5px; padding: 3px; } QListWidget::item { padding: 7px; }"));
    connect(textureList_, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem* current, QListWidgetItem*) { selectionChanged(current); });

    auto* right = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);
    preview_ = new ImagePreview(right);
    preview_->setObjectName(QStringLiteral("texturePreview"));
    preview_->setStyleSheet(QStringLiteral("border: 1px solid #c8cdd3; border-radius: 6px;"));
    rightLayout->addWidget(preview_, 1);
    details_ = new QLabel(QStringLiteral("Detail tekstur akan ditampilkan di sini."), right);
    details_->setTextFormat(Qt::PlainText);
    details_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    details_->setWordWrap(true);
    details_->setMinimumHeight(106);
    details_->setStyleSheet(QStringLiteral("background: white; border: 1px solid #d7dce1; border-radius: 5px; padding: 10px; color: #28313b;"));
    rightLayout->addWidget(details_);

    splitter->addWidget(textureList_);
    splitter->addWidget(right);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({350, 900});
    rootLayout->addWidget(splitter, 1);

    auto* footnote = new QLabel(QStringLiteral("Import yang dapat ditulis saat ini: TIM2 indexed 8-bit + CLUT RGBA32, 256 warna, satu mip level, header gambar 48 byte. Untuk tekstur yang berasal dari ZIP, gunakan 'Simpan semua tekstur berubah ke ZIP baru…'; semua TIM2 yang diubah dalam arsip sesi ini dikemas sekaligus dan sumber tidak ditimpa."), root);
    footnote->setWordWrap(true);
    footnote->setStyleSheet(QStringLiteral("color: #657285; font-size: 11px; padding: 3px;"));
    rootLayout->addWidget(footnote);
    return root;
}


