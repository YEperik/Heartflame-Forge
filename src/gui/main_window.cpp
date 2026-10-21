#include "main_window.hpp"

#include "ps2asset/archive/zip_reader.hpp"

#include <QAction>
#include <QByteArray>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPainter>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSaveFile>
#include <QDir>
#include <QSignalBlocker>
#include <QStringList>
#include <QSizePolicy>
#include <QColor>
#include <QAbstractItemView>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {
constexpr int kDocumentRole = Qt::UserRole + 1;
constexpr int kPictureRole = Qt::UserRole + 2;
constexpr qint64 kMaxDirectTextureFileBytes = 512LL * 1024LL * 1024LL;

std::vector<std::uint8_t> readFileBytes(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error(QStringLiteral("Tidak dapat membuka file: %1").arg(file.errorString()).toStdString());
    }
    if (file.size() < 0 || file.size() > kMaxDirectTextureFileBytes) {
        throw std::runtime_error("Ukuran file melewati batas keamanan 512 MiB");
    }
    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        throw std::runtime_error(QStringLiteral("Gagal membaca file: %1").arg(file.errorString()).toStdString());
    }
    std::vector<std::uint8_t> result(static_cast<std::size_t>(bytes.size()));
    if (!bytes.isEmpty()) {
        const auto* begin = reinterpret_cast<const std::uint8_t*>(bytes.constData());
        std::copy(begin, begin + bytes.size(), result.begin());
    }
    return result;
}

QString baseNameForSave(QString name) {
    name = QDir::fromNativeSeparators(name);
    const int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.mid(slash + 1);
    if (name.isEmpty()) name = QStringLiteral("texture.tm2");
    if (!name.endsWith(QStringLiteral(".tm2"), Qt::CaseInsensitive)) name += QStringLiteral(".tm2");
    return name;
}

QString safeExceptionText(const std::exception& error) {
    return QString::fromUtf8(error.what());
}
}  // namespace

ImagePreview::ImagePreview(QWidget* parent) : QWidget(parent) {
    setMinimumSize(320, 240);
    setAutoFillBackground(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ImagePreview::setImage(const QImage& image) {
    image_ = image;
    message_.clear();
    update();
}

void ImagePreview::setMessage(const QString& message) {
    image_ = QImage();
    message_ = message;
    update();
}

void ImagePreview::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(35, 38, 43));
    if (image_.isNull()) {
        painter.setPen(QColor(205, 210, 218));
        painter.drawText(rect().adjusted(16, 16, -16, -16), Qt::AlignCenter | Qt::TextWordWrap, message_);
        return;
    }

    const QRect area = rect().adjusted(18, 18, -18, -18);
    const QSize scaled = image_.size().scaled(area.size(), Qt::KeepAspectRatio);
    const QRect target(QPoint(area.center().x() - scaled.width() / 2,
                              area.center().y() - scaled.height() / 2), scaled);
    constexpr int tile = 12;
    const QColor light(75, 78, 84);
    const QColor dark(57, 60, 66);
    for (int y = target.top(); y <= target.bottom(); y += tile) {
        for (int x = target.left(); x <= target.right(); x += tile) {
            const bool parity = ((x - target.left()) / tile + (y - target.top()) / tile) % 2 == 0;
            painter.fillRect(QRect(x, y, std::min(tile, target.right() - x + 1),
                                   std::min(tile, target.bottom() - y + 1)), parity ? light : dark);
        }
    }
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.drawImage(target, image_);
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    buildUi();
    resize(1180, 760);
    setMinimumSize(900, 600);
    statusBar()->showMessage(QStringLiteral("Siap. Buka file TIM2 (.tm2) atau arsip ZIP."));
}

void MainWindow::buildUi() {
    setWindowTitle(QStringLiteral("PS2 Asset Workbench — Texture Module"));
    auto* toolbar = addToolBar(QStringLiteral("Tekstur"));
    toolbar->setMovable(false);
    QAction* openAction = toolbar->addAction(style()->standardIcon(QStyle::SP_DialogOpenButton), QStringLiteral("Buka"));
    openAction->setToolTip(QStringLiteral("Buka file .tm2 atau arsip .zip"));
    importAction_ = toolbar->addAction(style()->standardIcon(QStyle::SP_FileDialogStart), QStringLiteral("Import / Ganti"));
    importAction_->setToolTip(QStringLiteral("Import gambar PNG/BMP/JPEG untuk mengganti tekstur terpilih"));
    exportAction_ = toolbar->addAction(style()->standardIcon(QStyle::SP_DialogSaveButton), QStringLiteral("Export PNG"));
    exportAction_->setToolTip(QStringLiteral("Ekspor tekstur terpilih sebagai PNG"));
    saveAction_ = toolbar->addAction(style()->standardIcon(QStyle::SP_DialogSaveButton), QStringLiteral("Simpan TM2 sebagai…"));
    saveAction_->setToolTip(QStringLiteral("Simpan file TIM2 yang dipilih menjadi file .tm2 baru"));
    toolbar->addSeparator();
    QAction* aboutAction = toolbar->addAction(QStringLiteral("Tentang"));

    connect(openAction, &QAction::triggered, this, [this]() { openDialog(); });
    connect(importAction_, &QAction::triggered, this, [this]() { importSelectedTexture(); });
    connect(exportAction_, &QAction::triggered, this, [this]() { exportSelectedTexture(); });
    connect(saveAction_, &QAction::triggered, this, [this]() { saveSelectedTim2As(); });
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, QStringLiteral("Tentang PS2 Asset Workbench"),
            QStringLiteral("<b>PS2 Asset Workbench 0.2.0</b><br>Modul awal untuk inspeksi, ekspor, impor, dan penggantian tekstur TIM2.<br><br>Profil import tahap pertama: tekstur indexed 8-bit dengan palette RGBA32, 256 warna, satu mip level, dan header gambar 48 byte. File sumber tidak ditimpa otomatis."));
    });

    auto* root = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(10, 10, 10, 8);
    rootLayout->setSpacing(8);

    sourceLabel_ = new QLabel(QStringLiteral("Sumber: belum ada file dibuka"), root);
    sourceLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    sourceLabel_->setStyleSheet(QStringLiteral("color: #4c5968; padding: 3px 2px;"));
    rootLayout->addWidget(sourceLabel_);

    auto* splitter = new QSplitter(Qt::Horizontal, root);
    textureList_ = new QListWidget(splitter);
    textureList_->setMinimumWidth(270);
    textureList_->setMaximumWidth(440);
    textureList_->setSelectionMode(QAbstractItemView::SingleSelection);
    textureList_->setUniformItemSizes(false);
    textureList_->setAlternatingRowColors(true);
    connect(textureList_, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem* current, QListWidgetItem*) { selectionChanged(current); });

    auto* right = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);
    preview_ = new ImagePreview(right);
    preview_->setObjectName(QStringLiteral("texturePreview"));
    preview_->setStyleSheet(QStringLiteral("border: 1px solid #c8cdd3; border-radius: 4px;"));
    rightLayout->addWidget(preview_, 1);
    details_ = new QLabel(QStringLiteral("Detail tekstur akan ditampilkan di sini."), right);
    details_->setTextFormat(Qt::PlainText);
    details_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    details_->setWordWrap(true);
    details_->setMinimumHeight(86);
    details_->setStyleSheet(QStringLiteral("background: #f3f5f7; border: 1px solid #d7dce1; border-radius: 4px; padding: 8px; color: #28313b;"));
    rightLayout->addWidget(details_);

    splitter->addWidget(textureList_);
    splitter->addWidget(right);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({330, 820});
    rootLayout->addWidget(splitter, 1);

    auto* footnote = new QLabel(QStringLiteral("Catatan: penggantian saat ini mendukung TIM2 indexed 8-bit + palette RGBA32. Arsip ZIP dibuka untuk menelusuri/mengekspor; hasil edit dari ZIP disimpan sebagai .tm2 terpisah, bukan menulis ulang ZIP."), root);
    footnote->setWordWrap(true);
    footnote->setStyleSheet(QStringLiteral("color: #656e78; font-size: 11px; padding: 2px;"));
    rootLayout->addWidget(footnote);

    setCentralWidget(root);
    importAction_->setEnabled(false);
    exportAction_->setEnabled(false);
    saveAction_->setEnabled(false);
}

void MainWindow::openDialog() {
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Buka aset game"), currentPath_,
        QStringLiteral("Aset TIM2 dan arsip ZIP (*.tm2 *.TM2 *.zip *.ZIP);;TIM2 (*.tm2 *.TM2);;ZIP (*.zip *.ZIP);;Semua file (*.*)"));
    if (!path.isEmpty()) openPath(path);
}

void MainWindow::openPath(const QString& path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    try {
        if (suffix == QStringLiteral("tm2")) openTim2(path);
        else if (suffix == QStringLiteral("zip")) openZip(path);
        else throw std::runtime_error("Pilih file .tm2 atau .zip");
    } catch (const std::exception& error) {
        QMessageBox::critical(this, QStringLiteral("Gagal membuka aset"), safeExceptionText(error));
        statusBar()->showMessage(QStringLiteral("Gagal membuka aset."), 6000);
    }
}

void MainWindow::openTim2(const QString& path) {
    auto bytes = readFileBytes(path);
    auto parsed = std::make_unique<ps2asset::texture::Tim2File>(
        ps2asset::texture::Tim2File::parse(std::move(bytes)));
    documents_.clear();
    TextureDocument doc;
    doc.name = QFileInfo(path).fileName();
    doc.file = std::move(parsed);
    documents_.push_back(std::move(doc));
    currentPath_ = path;
    selectedDocument_ = -1;
    selectedPicture_ = -1;
    sourceLabel_->setText(QStringLiteral("Sumber: %1").arg(QDir::toNativeSeparators(path)));
    refreshList();
    statusBar()->showMessage(QStringLiteral("TIM2 dibuka: %1 gambar." ).arg(documents_.front().file->picture_count()), 5000);
}

void MainWindow::openZip(const QString& path) {
    if (!ps2asset::archive::zip_support_available()) {
        throw std::runtime_error("Fitur buka ZIP belum aktif pada build ini. Pasang libarchive development package lalu konfigurasi ulang CMake.");
    }
    auto archived = ps2asset::archive::read_tim2_entries_from_zip(path.toStdString());
    std::vector<TextureDocument> loaded;
    loaded.reserve(archived.size());
    QStringList rejected;
    for (auto& entry : archived) {
        try {
            TextureDocument doc;
            doc.name = QString::fromUtf8(entry.entry_name.c_str());
            doc.file = std::make_unique<ps2asset::texture::Tim2File>(
                ps2asset::texture::Tim2File::parse(std::move(entry.bytes)));
            loaded.push_back(std::move(doc));
        } catch (const std::exception& error) {
            rejected.push_back(QStringLiteral("%1: %2").arg(QString::fromUtf8(entry.entry_name.c_str()), safeExceptionText(error)));
        }
    }
    if (loaded.empty()) {
        throw std::runtime_error("Tidak ditemukan file .tm2 yang dapat dibaca pada ZIP tersebut");
    }
    documents_ = std::move(loaded);
    currentPath_ = path;
    selectedDocument_ = -1;
    selectedPicture_ = -1;
    sourceLabel_->setText(QStringLiteral("Arsip: %1 — %2 file TIM2" ).arg(QDir::toNativeSeparators(path)).arg(static_cast<qulonglong>(documents_.size())));
    refreshList();
    statusBar()->showMessage(QStringLiteral("ZIP dibuka: %1 file TIM2, %2 gambar." )
        .arg(static_cast<qulonglong>(documents_.size())).arg(textureList_->count()), 6000);
    if (!rejected.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Beberapa file dilewati"),
            QStringLiteral("%1 entri .tm2 tidak dapat diparse. Contoh:\n%2").arg(rejected.size()).arg(rejected.mid(0, 5).join(QLatin1Char('\n'))));
    }
}

void MainWindow::refreshList(int selectRow) {
    const int previousDocument = selectedDocument_;
    const int previousPicture = selectedPicture_;
    QSignalBlocker blocker(textureList_);
    textureList_->clear();
    int row = 0;
    int desiredRow = -1;
    for (std::size_t docIndex = 0; docIndex < documents_.size(); ++docIndex) {
        const auto& doc = documents_[docIndex];
        const auto& pics = doc.file->pictures();
        for (std::size_t pictureIndex = 0; pictureIndex < pics.size(); ++pictureIndex) {
            const auto& pic = pics[pictureIndex];
            QString label = QStringLiteral("%1  |  #%2  |  %3×%4\n%5%6")
                .arg(doc.name).arg(static_cast<qulonglong>(pictureIndex)).arg(pic.width).arg(pic.height)
                .arg(QString::fromStdString(pic.image_type_name()))
                .arg(doc.modified ? QStringLiteral("  • DIUBAH") : QString());
            auto* item = new QListWidgetItem(label, textureList_);
            item->setData(kDocumentRole, static_cast<int>(docIndex));
            item->setData(kPictureRole, static_cast<int>(pictureIndex));
            if (static_cast<int>(docIndex) == previousDocument && static_cast<int>(pictureIndex) == previousPicture) desiredRow = row;
            ++row;
        }
    }
    if (textureList_->count() > 0) {
        if (desiredRow < 0) desiredRow = std::clamp(selectRow, 0, textureList_->count() - 1);
        textureList_->setCurrentRow(desiredRow);
    }
    blocker.unblock();
    if (textureList_->count() > 0) selectionChanged(textureList_->currentItem());
    else updateSelection(-1, -1);
}

void MainWindow::selectionChanged(QListWidgetItem* item) {
    if (item == nullptr) {
        updateSelection(-1, -1);
        return;
    }
    bool docOk = false;
    bool picOk = false;
    const int docIndex = item->data(kDocumentRole).toInt(&docOk);
    const int pictureIndex = item->data(kPictureRole).toInt(&picOk);
    if (!docOk || !picOk) {
        updateSelection(-1, -1);
        return;
    }
    updateSelection(docIndex, pictureIndex);
}

void MainWindow::updateSelection(int documentIndex, int pictureIndex) {
    selectedDocument_ = documentIndex;
    selectedPicture_ = pictureIndex;
    const bool validSelection = hasSelection();
    importAction_->setEnabled(false);
    exportAction_->setEnabled(false);
    saveAction_->setEnabled(validSelection);
    if (!validSelection) {
        preview_->setMessage(QStringLiteral("Pilih tekstur untuk melihat pratinjau"));
        details_->setText(QStringLiteral("Detail tekstur akan ditampilkan di sini."));
        return;
    }

    const auto& doc = documents_[static_cast<std::size_t>(selectedDocument_)];
    const auto& pic = doc.file->pictures()[static_cast<std::size_t>(selectedPicture_)];
    QString info = QStringLiteral("File: %1\nGambar: %2 dari %3\nDimensi: %4 × %5 px\nFormat: %6 | PSM: %7 | Mipmap: %8\nData gambar: %9 byte | CLUT: %10 byte (%11 warna)\nImport/ganti: %12 | Status: %13")
        .arg(doc.name).arg(selectedPicture_ + 1).arg(doc.file->picture_count())
        .arg(pic.width).arg(pic.height).arg(QString::fromStdString(pic.image_type_name()))
        .arg(pic.psm()).arg(pic.mipmap_count).arg(pic.image_size).arg(pic.clut_size).arg(pic.clut_colors)
        .arg(pic.supports_rgba8_palette_replace() ? QStringLiteral("didukung") : QStringLiteral("belum didukung"))
        .arg(!doc.modified ? QStringLiteral("asli / belum diubah") :
             (doc.savedAs ? QStringLiteral("diubah — sudah disimpan sebagai file baru") : QStringLiteral("diubah — belum disimpan")));
    details_->setText(info);
    importAction_->setEnabled(pic.supports_rgba8_palette_replace());

    try {
        const auto decoded = doc.file->decode_picture(static_cast<std::size_t>(selectedPicture_));
        preview_->setImage(toQImage(decoded));
        exportAction_->setEnabled(true);
    } catch (const std::exception& error) {
        preview_->setMessage(QStringLiteral("Pratinjau belum didukung untuk format ini.\n\n%1")
                             .arg(safeExceptionText(error)));
        exportAction_->setEnabled(false);
    }
}

void MainWindow::exportSelectedTexture() {
    if (!hasSelection()) return;
    const auto& doc = documents_[static_cast<std::size_t>(selectedDocument_)];
    const auto& pic = doc.file->pictures()[static_cast<std::size_t>(selectedPicture_)];
    QString suggested = QFileInfo(doc.name).completeBaseName() + QStringLiteral("_pic%1.png").arg(selectedPicture_);
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Ekspor tekstur sebagai PNG"), suggested,
                                                       QStringLiteral("PNG (*.png)"));
    if (path.isEmpty()) return;
    try {
        const QImage image = toQImage(doc.file->decode_picture(static_cast<std::size_t>(selectedPicture_)));
        QString output = path;
        if (!output.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive)) output += QStringLiteral(".png");
        if (!image.save(output, "PNG")) throw std::runtime_error("Qt tidak dapat menulis PNG tujuan");
        statusBar()->showMessage(QStringLiteral("Tekstur %1×%2 diekspor: %3").arg(pic.width).arg(pic.height).arg(output), 6000);
    } catch (const std::exception& error) {
        QMessageBox::critical(this, QStringLiteral("Ekspor gagal"), safeExceptionText(error));
    }
}

void MainWindow::importSelectedTexture() {
    if (!hasSelection()) return;
    auto& doc = documents_[static_cast<std::size_t>(selectedDocument_)];
    const auto& pic = doc.file->pictures()[static_cast<std::size_t>(selectedPicture_)];
    if (!pic.supports_rgba8_palette_replace()) {
        QMessageBox::information(this, QStringLiteral("Format belum didukung"),
            QStringLiteral("Import tahap pertama hanya menerima TIM2 indexed 8-bit dengan palette RGBA32 256 warna, satu mip level, dan header 48 byte."));
        return;
    }
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Pilih gambar pengganti"), QString(),
        QStringLiteral("Gambar (*.png *.bmp *.jpg *.jpeg *.tif *.tiff);;Semua file (*.*)"));
    if (path.isEmpty()) return;

    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QSize announcedSize = reader.size();
    if (announcedSize.isValid() && static_cast<qint64>(announcedSize.width()) * announcedSize.height() > 64LL * 1024LL * 1024LL) {
        QMessageBox::critical(this, QStringLiteral("Import gagal"),
            QStringLiteral("Gambar melebihi batas keselamatan 64 megapiksel."));
        return;
    }
    QImage image = reader.read();
    if (image.isNull()) {
        QMessageBox::critical(this, QStringLiteral("Import gagal"),
            QStringLiteral("Gambar tidak dapat dibaca: %1").arg(reader.errorString()));
        return;
    }
    if (image.size() != QSize(pic.width, pic.height)) {
        const auto answer = QMessageBox::question(this, QStringLiteral("Sesuaikan dimensi?"),
            QStringLiteral("Gambar berukuran %1×%2, sedangkan target adalah %3×%4.\n\nSkalakan otomatis ke ukuran tekstur?")
                .arg(image.width()).arg(image.height()).arg(pic.width).arg(pic.height()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (answer != QMessageBox::Yes) return;
        image = image.scaled(QSize(pic.width, pic.height), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    try {
        doc.file->replace_picture_rgba8_palette(static_cast<std::size_t>(selectedPicture_), fromQImage(image));
        doc.modified = true;
        doc.savedAs = false;
        const int selectedRow = textureList_->currentRow();
        refreshList(selectedRow);
        updateSelection(selectedDocument_, selectedPicture_);
        statusBar()->showMessage(QStringLiteral("Tekstur diganti di memori. Gunakan 'Simpan TM2 sebagai…' untuk menulis hasil ke file baru."), 8000);
    } catch (const std::exception& error) {
        QMessageBox::critical(this, QStringLiteral("Penggantian tekstur gagal"), safeExceptionText(error));
    }
}

void MainWindow::saveSelectedTim2As() {
    if (!hasSelection()) return;
    const auto& doc = documents_[static_cast<std::size_t>(selectedDocument_)];
    const QString suggested = baseNameForSave(doc.name);
    QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Simpan TIM2 sebagai file baru"), suggested,
                                                 QStringLiteral("TIM2 (*.tm2)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(QStringLiteral(".tm2"), Qt::CaseInsensitive)) path += QStringLiteral(".tm2");
    if (QFileInfo::exists(path)) {
        const auto answer = QMessageBox::question(this, QStringLiteral("Konfirmasi timpa file"),
            QStringLiteral("File tujuan sudah ada. Timpa file tersebut?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) return;
    }
    const auto& bytes = doc.file->bytes();
    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, QStringLiteral("Penyimpanan gagal"), output.errorString());
        return;
    }
    const qint64 count = output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<qint64>(bytes.size()));
    if (count != static_cast<qint64>(bytes.size())) {
        output.cancelWriting();
        QMessageBox::critical(this, QStringLiteral("Penyimpanan gagal"), QStringLiteral("Tidak semua byte berhasil ditulis."));
        return;
    }
    if (!output.commit()) {
        QMessageBox::critical(this, QStringLiteral("Penyimpanan gagal"), output.errorString());
        return;
    }
    documents_[static_cast<std::size_t>(selectedDocument_)].savedAs = true;
    updateSelection(selectedDocument_, selectedPicture_);
    statusBar()->showMessage(QStringLiteral("TIM2 tersimpan: %1 (%2 byte). Sumber asli tidak diubah." ).arg(path).arg(static_cast<qulonglong>(bytes.size())), 7000);
}

QImage MainWindow::toQImage(const ps2asset::texture::RgbaImage& image) const {
    if (!image.valid()) throw std::runtime_error("Decoded RGBA image has an invalid buffer");
    QImage result(static_cast<int>(image.width), static_cast<int>(image.height), QImage::Format_RGBA8888);
    if (result.isNull()) throw std::runtime_error("Tidak cukup memori untuk membuat pratinjau gambar");
    const std::size_t bytesPerRow = static_cast<std::size_t>(image.width) * 4U;
    for (std::uint32_t y = 0; y < image.height; ++y) {
        auto* destination = result.scanLine(static_cast<int>(y));
        const auto* source = image.pixels.data() + static_cast<std::size_t>(y) * bytesPerRow;
        std::copy(source, source + bytesPerRow, destination);
    }
    return result;
}

ps2asset::texture::RgbaImage MainWindow::fromQImage(const QImage& input) const {
    const QImage image = input.convertToFormat(QImage::Format_RGBA8888);
    if (image.isNull() || image.width() <= 0 || image.height() <= 0) throw std::runtime_error("Gambar impor tidak valid");
    ps2asset::texture::RgbaImage result;
    result.width = static_cast<std::uint32_t>(image.width());
    result.height = static_cast<std::uint32_t>(image.height());
    const std::size_t bytesPerRow = static_cast<std::size_t>(result.width) * 4U;
    result.pixels.resize(bytesPerRow * static_cast<std::size_t>(result.height));
    for (int y = 0; y < image.height(); ++y) {
        const auto* source = image.constScanLine(y);
        auto* destination = result.pixels.data() + static_cast<std::size_t>(y) * bytesPerRow;
        std::copy(source, source + bytesPerRow, destination);
    }
    if (!result.valid()) throw std::runtime_error("Dimensi gambar melebihi batas yang diterima modul tekstur");
    return result;
}

bool MainWindow::hasSelection() const noexcept {
    if (selectedDocument_ < 0 || selectedPicture_ < 0) return false;
    const auto doc = static_cast<std::size_t>(selectedDocument_);
    if (doc >= documents_.size() || !documents_[doc].file) return false;
    return static_cast<std::size_t>(selectedPicture_) < documents_[doc].file->pictures().size();
}
