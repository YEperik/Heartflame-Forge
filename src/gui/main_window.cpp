#include "main_window.hpp"

#include "ps2asset/archive/zip_reader.hpp"
#include "ps2asset/inspect.hpp"
#include "ps2asset/model/inspect.hpp"
#include "ps2asset/workbench/module_catalog.hpp"
#include "modules/audio_page.hpp"
#include "modules/video_page.hpp"
#include "modules/animation_page.hpp"
#include "modules/text_page.hpp"
#include "modules/model_export_page.hpp"

#include <QAction>
#include <QByteArray>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QIcon>
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
#include <QDebug>
#include <QDateTime>
#include <QDockWidget>
#include <QKeySequence>
#include <QSignalBlocker>
#include <QStringList>
#include <QSizePolicy>
#include <QColor>
#include <QCloseEvent>
#include <QAbstractItemView>
#include <QApplication>
#include <QCoreApplication>
#include <QFontDatabase>
#include <QGridLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QStandardPaths>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QFrame>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <functional>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <utility>

namespace {
constexpr int kDocumentRole = Qt::UserRole + 1;
constexpr int kPictureRole = Qt::UserRole + 2;
constexpr qint64 kMaxDirectTextureFileBytes = 512LL * 1024LL * 1024LL;
constexpr qint64 kMaxBinaryInspectBytes = 64LL * 1024LL * 1024LL;
constexpr std::size_t kMaxHexPreviewBytes = 8192U;

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
}

void MainWindow::buildUi() {
    setWindowTitle(QStringLiteral("Heartflame Forge 1.3.0  |  Asset Workshop"));
    setWindowIcon(QIcon(QStringLiteral(":/branding/heartflame-forge.png")));
    resize(1440, 900);
    setMinimumSize(1080, 680);

    auto* openAction = new QAction(style()->standardIcon(QStyle::SP_DialogOpenButton), QStringLiteral("Buka aset…"), this);
    openAction->setShortcut(QKeySequence::Open);
    openAction->setStatusTip(QStringLiteral("Buka file TIM2 atau arsip ZIP"));
    auto* inspectAction = new QAction(QStringLiteral("Periksa binary…"), this);
    inspectAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+I")));
    auto* exitAction = new QAction(QStringLiteral("Keluar"), this);
    exitAction->setShortcut(QKeySequence::Quit);
    importAction_ = new QAction(style()->standardIcon(QStyle::SP_FileDialogStart), QStringLiteral("Import / Ganti"), this);
    importAction_->setToolTip(QStringLiteral("Import gambar PNG/BMP/JPEG/TIFF untuk mengganti tekstur terpilih"));
    exportAction_ = new QAction(style()->standardIcon(QStyle::SP_DialogSaveButton), QStringLiteral("Export PNG"), this);
    exportAction_->setToolTip(QStringLiteral("Ekspor tekstur terpilih sebagai PNG"));
    saveAction_ = new QAction(QStringLiteral("Simpan TM2 sebagai…"), this);
    saveAction_->setToolTip(QStringLiteral("Simpan TIM2 hasil edit sebagai file baru"));
    saveZipAction_ = new QAction(QStringLiteral("Simpan semua tekstur berubah ke ZIP baru…"), this);
    saveZipAction_->setObjectName(QStringLiteral("saveZipAction"));
    saveZipAction_->setToolTip(QStringLiteral("Mengganti semua entry tekstur yang telah diedit dari arsip ini, lalu menyimpan ZIP baru; arsip sumber tidak diubah"));
    saveZipAction_->setEnabled(false);

    auto* fileMenu = menuBar()->addMenu(QStringLiteral("Berkas"));
    fileMenu->addAction(openAction);
    fileMenu->addAction(inspectAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);
    auto* editMenu = menuBar()->addMenu(QStringLiteral("Tekstur"));
    editMenu->addAction(importAction_);
    editMenu->addAction(exportAction_);
    editMenu->addAction(saveAction_);
    editMenu->addAction(saveZipAction_);
    auto* viewMenu = menuBar()->addMenu(QStringLiteral("Tampilan"));
    auto* homeAction = viewMenu->addAction(QStringLiteral("Ikhtisar / Dashboard"));
    auto* texturesAction = viewMenu->addAction(QStringLiteral("Texture Workshop"));
    auto* archiveAction = viewMenu->addAction(QStringLiteral("Archive Browser"));
    auto* binaryAction = viewMenu->addAction(QStringLiteral("Binary Inspector"));
    auto* helpMenu = menuBar()->addMenu(QStringLiteral("Bantuan"));
    auto* aboutAction = helpMenu->addAction(QStringLiteral("Tentang Heartflame Forge"));

    auto* toolbar = addToolBar(QStringLiteral("Aksi Utama"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->addAction(openAction);
    toolbar->addSeparator();
    toolbar->addAction(importAction_);
    toolbar->addAction(exportAction_);
    toolbar->addAction(saveAction_);
    toolbar->addAction(saveZipAction_);
    toolbar->addSeparator();
    toolbar->addAction(inspectAction);

    connect(openAction, &QAction::triggered, this, [this]() { openDialog(); });
    connect(inspectAction, &QAction::triggered, this, [this]() { inspectBinaryDialog(); });
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    connect(importAction_, &QAction::triggered, this, [this]() { importSelectedTexture(); });
    connect(exportAction_, &QAction::triggered, this, [this]() { exportSelectedTexture(); });
    connect(saveAction_, &QAction::triggered, this, [this]() { saveSelectedTim2As(); });
    connect(saveZipAction_, &QAction::triggered, this, [this]() { saveSelectedTextureToNewZip(); });
    connect(homeAction, &QAction::triggered, this, [this]() { selectModule(DashboardModule); });
    connect(texturesAction, &QAction::triggered, this, [this]() { selectModule(TextureModule); });
    connect(archiveAction, &QAction::triggered, this, [this]() { selectModule(ArchiveModule); });
    connect(binaryAction, &QAction::triggered, this, [this]() { selectModule(BinaryModule); });
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, QStringLiteral("Tentang Heartflame Forge"),
            QStringLiteral("<b>Heartflame Forge 1.3.0</b><br>Desktop workbench modular C++17/CMake untuk eksplorasi aset game PS2.<br><br>Workspace terpisah: Texture, Archive, Binary, Model Inspector, Model Export GLB (Assimp opsional), Animation, Audio, Video/Stream, dan Text/Script.<br>Parser inti tetap terpisah dari GUI dan plugin Qt dapat menambahkan halaman baru.<br><br>Catatan: mesh/animasi custom .mdl, codec proprietary SDPK/ADSC, dan decoder PSS belum diimplementasikan. Setiap halaman menandai fitur yang hanya berupa riset."));
    });

    auto* root = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* sidebar = new QWidget(root);
    sidebar->setObjectName(QStringLiteral("moduleSidebar"));
    sidebar->setMinimumWidth(210);
    sidebar->setMaximumWidth(260);
    sidebar->setStyleSheet(QStringLiteral("QWidget#moduleSidebar { background: #17202d; color: #e9eef5; } QLabel { color: #e9eef5; } QListWidget { background: #17202d; border: none; color: #e9eef5; outline: none; padding: 8px; } QListWidget::item { padding: 11px 10px; border-radius: 5px; margin: 2px 0; } QListWidget::item:selected { background: #315a84; color: white; } QListWidget::item:hover:!selected { background: #243246; }"));
    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(10, 18, 10, 12);
    sidebarLayout->setSpacing(10);
    auto* brand = new QLabel(QStringLiteral("PS2 ASSET\nWORKBENCH"), sidebar);
    brand->setStyleSheet(QStringLiteral("font-size: 19px; font-weight: 700; letter-spacing: 1px; padding: 4px 8px 10px 8px;"));
    sidebarLayout->addWidget(brand);
    auto* modulesLabel = new QLabel(QStringLiteral("WORKSPACES"), sidebar);
    modulesLabel->setStyleSheet(QStringLiteral("font-size: 10px; font-weight: 700; color: #9fb0c4; padding-left: 8px;"));
    sidebarLayout->addWidget(modulesLabel);
    moduleList_ = new QListWidget(sidebar);
    moduleList_->setObjectName(QStringLiteral("moduleNavigation"));
    moduleList_->setFrameShape(QFrame::NoFrame);
    moduleList_->setSpacing(1);
    const auto& moduleDescriptors = ps2asset::workbench::builtin_modules();
    for (const auto& descriptor : moduleDescriptors) {
        QStyle::StandardPixmap pixmap = QStyle::SP_FileIcon;
        if (descriptor.id == "dashboard") pixmap = QStyle::SP_DesktopIcon;
        else if (descriptor.id == "texture") pixmap = QStyle::SP_FileDialogContentsView;
        else if (descriptor.id == "archive") pixmap = QStyle::SP_DirOpenIcon;
        else if (descriptor.id == "binary") pixmap = QStyle::SP_FileDialogListView;
        else if (descriptor.id == "model-inspector") pixmap = QStyle::SP_ComputerIcon;
        else if (descriptor.id == "model-export-glb") pixmap = QStyle::SP_DialogSaveButton;
        else if (descriptor.id == "animation") pixmap = QStyle::SP_DesktopIcon;
        else if (descriptor.id == "audio") pixmap = QStyle::SP_MediaVolume;
        else if (descriptor.id == "video") pixmap = QStyle::SP_MediaPlay;
        auto* item = new QListWidgetItem(style()->standardIcon(pixmap), QString::fromStdString(descriptor.display_name));
        item->setData(Qt::UserRole, QString::fromStdString(descriptor.id));
        item->setToolTip(QString::fromStdString(descriptor.summary + "\nStatus: " +
            ps2asset::workbench::readiness_name(descriptor.readiness)));
        moduleList_->addItem(item);
    }
    sidebarLayout->addWidget(moduleList_, 1);
    auto* sidebarFooter = new QLabel(QStringLiteral("C++17  •  CMake\nPlugin API v1.0"), sidebar);
    sidebarFooter->setStyleSheet(QStringLiteral("color: #9fb0c4; font-size: 10px; padding: 8px;"));
    sidebarLayout->addWidget(sidebarFooter);
    rootLayout->addWidget(sidebar);

    moduleStack_ = new QStackedWidget(root);
    moduleStack_->setObjectName(QStringLiteral("moduleStack"));
    moduleStack_->setStyleSheet(QStringLiteral("QStackedWidget { background: #f4f6f9; }"));
    moduleStack_->addWidget(buildDashboardPage());
    moduleStack_->addWidget(buildTexturePage());
    moduleStack_->addWidget(buildArchiveBrowserPage());
    moduleStack_->addWidget(buildBinaryInspectorPage());
    moduleStack_->addWidget(buildModelInspectorPage());
    moduleStack_->addWidget(new ps2asset::gui::ModelExportPage(moduleStack_));
    moduleStack_->addWidget(new ps2asset::gui::AnimationPage(moduleStack_));
    moduleStack_->addWidget(new ps2asset::gui::AudioPage(moduleStack_));
    moduleStack_->addWidget(new ps2asset::gui::VideoPage(moduleStack_));
    moduleStack_->addWidget(new ps2asset::gui::TextPage(moduleStack_));
    rootLayout->addWidget(moduleStack_, 1);
    setCentralWidget(root);

    logDock_ = new QDockWidget(QStringLiteral("Diagnostics / Activity Log"), this);
    logDock_->setObjectName(QStringLiteral("diagnosticsDock"));
    logDock_->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
    logView_ = new QPlainTextEdit(logDock_);
    logView_->setReadOnly(true);
    logView_->setMaximumBlockCount(2000);
    logView_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    logView_->setStyleSheet(QStringLiteral("QPlainTextEdit { background: #151a22; color: #d8e3f0; border: none; padding: 7px; }"));
    logDock_->setWidget(logView_);
    addDockWidget(Qt::BottomDockWidgetArea, logDock_);
    logDock_->hide();
    viewMenu->addAction(logDock_->toggleViewAction());

    connect(moduleList_, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0 || row >= moduleStack_->count()) return;
        moduleStack_->setCurrentIndex(row);
        const bool texturePage = row == TextureModule;
        if (!texturePage) {
            importAction_->setEnabled(false);
            exportAction_->setEnabled(false);
            saveAction_->setEnabled(false);
            saveZipAction_->setEnabled(false);
        } else if (textureList_ != nullptr) {
            selectionChanged(textureList_->currentItem());
        }
    });

    loadExtensionModules();
    moduleList_->setCurrentRow(DashboardModule);
    selectModule(DashboardModule);
    statusBar()->showMessage(QStringLiteral("Siap. Buka file .tm2 atau ZIP; gunakan Binary Inspector untuk pemeriksaan header."));
    if (!pluginManager_.errors().isEmpty()) {
        statusBar()->showMessage(QStringLiteral("Aplikasi siap; %1 extension dilewati. Periksa log plugin pada dokumentasi atau folder plugins.")
                                     .arg(pluginManager_.errors().size()), 10000);
    }
}

QWidget* MainWindow::buildDashboardPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(28, 26, 28, 24);
    layout->setSpacing(18);

    auto* heading = new QLabel(QStringLiteral("Heartflame Forge"), page);
    heading->setStyleSheet(QStringLiteral("font-size: 28px; font-weight: 700; color: #1d2a3a;"));
    auto* subheading = new QLabel(QStringLiteral("Ruang kerja reverse engineering untuk aset PlayStation 2"), page);
    subheading->setStyleSheet(QStringLiteral("font-size: 14px; color: #59687a;"));
    layout->addWidget(heading);
    layout->addWidget(subheading);

    dashboardSummary_ = new QLabel(page);
    dashboardSummary_->setWordWrap(true);
    dashboardSummary_->setStyleSheet(QStringLiteral("background: white; border: 1px solid #dfe5ed; border-radius: 8px; padding: 18px; color: #2c3949; font-size: 14px;"));
    layout->addWidget(dashboardSummary_);

    auto* cards = new QGridLayout();
    cards->setHorizontalSpacing(14);
    cards->setVerticalSpacing(14);
    const auto makeCard = [page](const QString& title, const QString& body, const QString& buttonText,
                                 const std::function<void()>& action) {
        auto* card = new QFrame(page);
        card->setFrameShape(QFrame::StyledPanel);
        card->setStyleSheet(QStringLiteral("QFrame { background: white; border: 1px solid #dfe5ed; border-radius: 8px; }"));
        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 16, 16, 16);
        auto* titleLabel = new QLabel(title, card);
        titleLabel->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 700; color: #263548; border: none;"));
        auto* bodyLabel = new QLabel(body, card);
        bodyLabel->setWordWrap(true);
        bodyLabel->setStyleSheet(QStringLiteral("color: #617082; border: none;"));
        auto* button = new QPushButton(buttonText, card);
        button->setMinimumHeight(34);
        QObject::connect(button, &QPushButton::clicked, page, [action]() { action(); });
        cardLayout->addWidget(titleLabel);
        cardLayout->addWidget(bodyLabel, 1);
        cardLayout->addWidget(button, 0, Qt::AlignLeft);
        return card;
    };
    cards->addWidget(makeCard(QStringLiteral("Texture Workshop"),
        QStringLiteral("Buka file TIM2 atau ZIP, lihat pratinjau, ekspor PNG, ganti palet/tekstur yang didukung, lalu simpan ke file baru."),
        QStringLiteral("Buka texture workspace"), [this]() { selectModule(TextureModule); }), 0, 0);
    cards->addWidget(makeCard(QStringLiteral("Binary Inspector"),
        QStringLiteral("Periksa signature, ukuran, kandidat format, dump hex awal, dan pencarian string ASCII pada berkas sampai 64 MiB."),
        QStringLiteral("Pilih berkas untuk inspeksi"), [this]() { selectModule(BinaryModule); inspectBinaryDialog(); }), 0, 1);
    cards->addWidget(makeCard(QStringLiteral("Extension API"),
        QStringLiteral("Modul opsional dapat dimuat dari folder plugins tanpa menautkan format spesifik ke core library. API versi 1.0 tersedia di include/ps2asset/workbench."),
        QStringLiteral("Lihat extension"), [this]() {
            if (moduleList_->count() > BuiltInModuleCount) selectModule(BuiltInModuleCount);
            else QMessageBox::information(this, QStringLiteral("Extension API"),
                QStringLiteral("Belum ada plugin eksternal ditemukan. Baca plugins/README.md untuk panduan membuat modul."));
        }), 1, 0);
    cards->addWidget(makeCard(QStringLiteral("Archive Browser"),
        QStringLiteral("Lihat daftar entri arsip ZIP, tipe file, dan ukuran tanpa mengekstrak seluruh isi. Entri TIM2 dapat diteruskan ke Texture Workshop."),
        QStringLiteral("Jelajahi arsip ZIP"), [this]() { selectModule(ArchiveModule); openArchiveDialog(); }), 1, 1);
    layout->addLayout(cards);

    auto* disclaimer = new QLabel(QStringLiteral("Catatan engineering: workspace Model, Animation, Audio, Video, dan Text sudah dipisah per fitur. Beberapa masih read-only atau menunggu riset format; tidak ada decoder otomatis untuk format proprietary hanya berdasarkan signature."), page);
    disclaimer->setWordWrap(true);
    disclaimer->setStyleSheet(QStringLiteral("color: #657285; padding: 8px 2px;"));
    layout->addWidget(disclaimer);
    layout->addStretch(1);
    updateDashboardSummary();
    return page;
}

void MainWindow::selectModule(int row) {
    if (moduleList_ == nullptr || row < 0 || row >= moduleList_->count()) return;
    moduleList_->setCurrentRow(row);
}

void MainWindow::loadExtensionModules() {
    QStringList directories;
    directories.push_back(QCoreApplication::applicationDirPath() + QStringLiteral("/plugins"));
    const QString appDataDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!appDataDirectory.isEmpty()) directories.push_back(appDataDirectory + QStringLiteral("/plugins"));
    directories.removeDuplicates();
    pluginManager_.discover(directories, moduleStack_);
    for (const auto& module : pluginManager_.modules()) {
        moduleStack_->addWidget(module.widget);
        auto* item = new QListWidgetItem(style()->standardIcon(QStyle::SP_DirIcon), module.name, moduleList_);
        item->setToolTip(module.description + QStringLiteral("\n") + module.filePath);
        appendLog(QStringLiteral("[Plugin] Loaded %1 (%2) from %3").arg(module.name).arg(module.id).arg(module.filePath));
    }
    for (const QString& error : pluginManager_.errors()) {
        qWarning().noquote() << QStringLiteral("[Plugin] %1").arg(error);
        appendLog(QStringLiteral("[Plugin] ERROR: %1").arg(error));
    }
}

void MainWindow::updateDashboardSummary() {
    if (dashboardSummary_ == nullptr) return;
    std::size_t pictureCount = 0;
    std::size_t modifiedCount = 0;
    for (const auto& doc : documents_) {
        if (doc.file) pictureCount += doc.file->pictures().size();
        if (doc.modified) ++modifiedCount;
    }
    QString text;
    if (documents_.empty()) {
        text = QStringLiteral("Belum ada aset dibuka.\nMulai dengan file .tm2 tunggal atau arsip ZIP yang berisi TIM2. Berkas asli tidak ditimpa oleh proses import/ganti tekstur.");
    } else {
        text = QStringLiteral("Aset aktif: %1\nDokumen TIM2: %2    •    Gambar: %3    •    Dokumen berubah: %4\nSumber: %5")
            .arg(currentPath_.isEmpty() ? QStringLiteral("(belum ditentukan)") : QDir::toNativeSeparators(currentPath_))
            .arg(static_cast<qulonglong>(documents_.size()))
            .arg(static_cast<qulonglong>(pictureCount))
            .arg(static_cast<qulonglong>(modifiedCount));
    }
    dashboardSummary_->setText(text);
}

void MainWindow::appendLog(const QString& message) {
    if (logView_ == nullptr) return;
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    logView_->appendPlainText(QStringLiteral("[%1] %2").arg(timestamp, message));
}

void MainWindow::openDialog() {
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Buka aset game"), currentPath_,
        QStringLiteral("Aset TIM2 dan arsip ZIP (*.tm2 *.TM2 *.zip *.ZIP);;TIM2 (*.tm2 *.TM2);;ZIP (*.zip *.ZIP);;Semua file (*.*)"));
    if (!path.isEmpty()) openPath(path);
}

void MainWindow::openPath(const QString& path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    try {
        if (suffix != QStringLiteral("tm2") && suffix != QStringLiteral("zip")) {
            throw std::runtime_error("Pilih file .tm2 atau .zip");
        }
        // Parse/validate the candidate source before asking whether to discard edits.
        // openTim2/openZip commit new documents only after parsing succeeds and
        // after the user confirms the discard prompt.
        if (suffix == QStringLiteral("tm2")) openTim2(path);
        else openZip(path);
    } catch (const std::exception& error) {
        appendLog(QStringLiteral("[Open] ERROR: %1").arg(safeExceptionText(error)));
        QMessageBox::critical(this, QStringLiteral("Gagal membuka aset"), safeExceptionText(error));
        statusBar()->showMessage(QStringLiteral("Gagal membuka aset."), 6000);
    }
}

bool MainWindow::confirmDiscardUnsavedChanges() {
    QStringList changed;
    for (const auto& document : documents_) {
        if (document.modified && !document.savedAs) changed.push_back(document.name);
    }
    if (changed.isEmpty()) return true;
    QString message = QStringLiteral("Ada %1 dokumen TIM2 dengan perubahan yang belum disimpan.").arg(changed.size());
    if (!changed.isEmpty()) message += QStringLiteral("\n\n%1").arg(changed.mid(0, 6).join(QLatin1Char('\n')));
    if (changed.size() > 6) message += QStringLiteral("\n… dan %1 lainnya").arg(changed.size() - 6);
    message += QStringLiteral("\n\nBuang perubahan dan lanjutkan?");
    const auto answer = QMessageBox::warning(this, QStringLiteral("Perubahan belum disimpan"), message,
        QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel);
    return answer == QMessageBox::Discard;
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (confirmDiscardUnsavedChanges()) event->accept();
    else event->ignore();
}

void MainWindow::openTim2(const QString& path) {
    auto bytes = readFileBytes(path);
    auto parsed = std::make_unique<ps2asset::texture::Tim2File>(
        ps2asset::texture::Tim2File::parse(std::move(bytes)));
    if (!confirmDiscardUnsavedChanges()) return;
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
    selectModule(TextureModule);
    updateDashboardSummary();
    appendLog(QStringLiteral("[Texture] Loaded %1 (%2 pictures)").arg(path).arg(documents_.front().file->picture_count()));
    statusBar()->showMessage(QStringLiteral("TIM2 dibuka: %1 gambar.").arg(documents_.front().file->picture_count()), 5000);
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
            doc.sourceArchivePath = QFileInfo(path).absoluteFilePath();
            doc.sourceArchiveEntry = doc.name;
            doc.sourceArchiveEntryRaw = entry.entry_name;
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
    if (!confirmDiscardUnsavedChanges()) return;
    documents_ = std::move(loaded);
    currentPath_ = path;
    selectedDocument_ = -1;
    selectedPicture_ = -1;
    sourceLabel_->setText(QStringLiteral("Arsip: %1 — %2 file TIM2").arg(QDir::toNativeSeparators(path)).arg(static_cast<qulonglong>(documents_.size())));
    refreshList();
    selectModule(TextureModule);
    updateDashboardSummary();
    appendLog(QStringLiteral("[Archive] Loaded %1: %2 TIM2 files, %3 pictures")
        .arg(path).arg(static_cast<qulonglong>(documents_.size())).arg(textureList_->count()));
    statusBar()->showMessage(QStringLiteral("ZIP dibuka: %1 file TIM2, %2 gambar.")
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
    saveZipAction_->setEnabled(false);
    if (!validSelection) {
        preview_->setMessage(QStringLiteral("Pilih tekstur untuk melihat pratinjau"));
        details_->setText(QStringLiteral("Detail tekstur akan ditampilkan di sini."));
        return;
    }

    const auto& doc = documents_[static_cast<std::size_t>(selectedDocument_)];
    const auto& pic = doc.file->pictures()[static_cast<std::size_t>(selectedPicture_)];
    const bool hasModifiedEntryFromArchive = !doc.sourceArchivePath.isEmpty() &&
        std::any_of(documents_.begin(), documents_.end(), [&](const TextureDocument& candidate) {
            return candidate.modified && candidate.sourceArchivePath == doc.sourceArchivePath &&
                   !candidate.sourceArchiveEntryRaw.empty();
        });
    saveZipAction_->setEnabled(hasModifiedEntryFromArchive);
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
        appendLog(QStringLiteral("[Texture] Exported %1×%2 to %3").arg(pic.width).arg(pic.height).arg(output));
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
                .arg(image.width()).arg(image.height()).arg(pic.width).arg(pic.height),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (answer != QMessageBox::Yes) return;
        image = image.scaled(QSize(pic.width, pic.height), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    try {
        doc.file->replace_picture_rgba8_palette(static_cast<std::size_t>(selectedPicture_), fromQImage(image));
        doc.modified = true;
        doc.savedAs = false;
        appendLog(QStringLiteral("[Texture] Replaced picture %1 in %2 (pending save-as)").arg(selectedPicture_).arg(doc.name));
        updateDashboardSummary();
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
    updateDashboardSummary();
    updateSelection(selectedDocument_, selectedPicture_);
    appendLog(QStringLiteral("[Texture] Saved modified TIM2 as %1 (%2 bytes)").arg(path).arg(static_cast<qulonglong>(bytes.size())));
    statusBar()->showMessage(QStringLiteral("TIM2 tersimpan: %1 (%2 byte). Sumber asli tidak diubah.").arg(path).arg(static_cast<qulonglong>(bytes.size())), 7000);
}

void MainWindow::saveSelectedTextureToNewZip() {
    if (!hasSelection()) return;
    const auto& selected = documents_[static_cast<std::size_t>(selectedDocument_)];
    if (selected.sourceArchivePath.isEmpty() || selected.sourceArchiveEntry.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Sumber ZIP tidak tersedia"),
            QStringLiteral("Tekstur ini tidak dibuka dari entry ZIP. Buka arsip melalui Archive Browser atau Texture Workshop terlebih dahulu."));
        return;
    }
    if (!ps2asset::archive::zip_support_available()) {
        QMessageBox::critical(this, QStringLiteral("ZIP tidak tersedia"),
            QStringLiteral("Build ini tidak memiliki dukungan libarchive. Pasang libarchive development package dan build ulang."));
        return;
    }

    std::vector<std::size_t> modifiedDocumentIndices;
    std::vector<ps2asset::archive::ZipEntryReplacement> replacements;
    for (std::size_t i = 0; i < documents_.size(); ++i) {
        const auto& candidate = documents_[i];
        if (!candidate.modified || candidate.sourceArchivePath != selected.sourceArchivePath ||
            candidate.sourceArchiveEntryRaw.empty() || !candidate.file) continue;
        // Validate each edited container before starting the archive rebuild.
        try {
            (void)ps2asset::texture::Tim2File::parse(candidate.file->bytes());
        } catch (const std::exception& error) {
            QMessageBox::critical(this, QStringLiteral("TIM2 tidak valid"),
                QStringLiteral("Tekstur %1 gagal divalidasi:\n%2").arg(candidate.name, safeExceptionText(error)));
            appendLog(QStringLiteral("[Archive] Batch pre-validation failed for %1: %2")
                .arg(candidate.name, safeExceptionText(error)));
            return;
        }
        replacements.push_back({candidate.sourceArchiveEntryRaw, candidate.file->bytes()});
        modifiedDocumentIndices.push_back(i);
    }
    if (replacements.empty()) {
        QMessageBox::information(this, QStringLiteral("Belum ada perubahan"),
            QStringLiteral("Ganti setidaknya satu tekstur terlebih dahulu sebelum membuat ZIP hasil modifikasi."));
        return;
    }

    const QFileInfo sourceInfo(selected.sourceArchivePath);
    const QString suggested = sourceInfo.absolutePath() + QLatin1Char('/') +
        sourceInfo.completeBaseName() + QStringLiteral("_modded.zip");
    QString outputPath = QFileDialog::getSaveFileName(this, QStringLiteral("Simpan tekstur yang diubah ke ZIP baru"),
        suggested, QStringLiteral("ZIP archive (*.zip)"));
    if (outputPath.isEmpty()) return;
    if (!outputPath.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) outputPath += QStringLiteral(".zip");

    const QString sourceCanonical = QFileInfo(selected.sourceArchivePath).canonicalFilePath();
    const QString outputAbsolute = QFileInfo(outputPath).absoluteFilePath();
    const QString outputCanonical = QFileInfo(outputPath).canonicalFilePath();
    if ((!sourceCanonical.isEmpty() && outputCanonical == sourceCanonical) ||
        outputAbsolute == QFileInfo(selected.sourceArchivePath).absoluteFilePath()) {
        QMessageBox::critical(this, QStringLiteral("Arsip sumber dilindungi"),
            QStringLiteral("Pilih nama/path lain. Arsip sumber tidak boleh menjadi file tujuan."));
        return;
    }
    if (QFileInfo::exists(outputPath)) {
        QMessageBox::warning(this, QStringLiteral("File tujuan sudah ada"),
            QStringLiteral("File tujuan sudah ada. Untuk mencegah kehilangan data, pilih nama ZIP baru yang belum digunakan."));
        return;
    }

    bool outputCreated = false;
    try {
        ps2asset::archive::replace_zip_entries_bytes(selected.sourceArchivePath.toStdString(), replacements,
                                                       outputPath.toStdString());
        outputCreated = true;

        // Confirm every edited entry survives in the new archive before reporting success.
        const auto outputEntries = ps2asset::archive::list_zip_entries(outputPath.toStdString());
        for (const auto& replacement : replacements) {
            const auto target = std::find_if(outputEntries.begin(), outputEntries.end(), [&](const auto& entry) {
                return entry.path == replacement.entry_path && entry.is_regular_file;
            });
            if (target == outputEntries.end()) {
                throw std::runtime_error("Verifikasi output gagal: entry tidak ditemukan pada ZIP yang ditulis: " + replacement.entry_path);
            }
        }

        for (const auto index : modifiedDocumentIndices) documents_[index].savedAs = true;
        updateDashboardSummary();
        updateSelection(selectedDocument_, selectedPicture_);
        appendLog(QStringLiteral("[Archive] Replaced %1 edited texture entries in new ZIP %2; source preserved")
            .arg(static_cast<qulonglong>(replacements.size())).arg(outputPath));
        statusBar()->showMessage(QStringLiteral("ZIP baru tersimpan: %1 (%2 tekstur diganti)")
            .arg(outputPath).arg(static_cast<qulonglong>(replacements.size())), 8000);
        QMessageBox::information(this, QStringLiteral("ZIP berhasil dibuat"),
            QStringLiteral("%1 entry tekstur yang diedit telah diganti dalam arsip baru.\n\nOutput:\n%2\n\nArsip sumber tidak diubah.")
                .arg(static_cast<qulonglong>(replacements.size())).arg(outputPath));
    } catch (const std::exception& error) {
        QString message = safeExceptionText(error);
        if (outputCreated) {
            message += QStringLiteral("\n\nZIP output sudah terbuat, tetapi validasi lanjutan gagal. File tidak dihapus otomatis agar tidak berisiko menghapus file yang diubah proses lain. Periksa output secara manual: ") + outputPath;
        }
        QMessageBox::critical(this, QStringLiteral("Gagal memverifikasi ZIP modifikasi"), message);
        appendLog(QStringLiteral("[Archive] Batch repack/verification failed: %1").arg(safeExceptionText(error)));
    }
}

void MainWindow::openArchiveDialog() {
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Buka arsip ZIP"), currentPath_,
                                                       QStringLiteral("ZIP archive (*.zip *.ZIP);;Semua file (*.*)"));
    if (path.isEmpty()) return;
    try {
        loadArchiveListing(path);
        appendLog(QStringLiteral("[Archive] Indexed %1 (%2 entries)").arg(path).arg(static_cast<qulonglong>(archiveEntries_.size())));
        selectModule(ArchiveModule);
        statusBar()->showMessage(QStringLiteral("Arsip selesai diindeks: %1 entri").arg(static_cast<qulonglong>(archiveEntries_.size())), 6000);
    } catch (const std::exception& error) {
        QMessageBox::critical(this, QStringLiteral("Gagal membaca arsip"), safeExceptionText(error));
    }
}

void MainWindow::loadArchiveListing(const QString& path) {
    if (!ps2asset::archive::zip_support_available()) {
        throw std::runtime_error("Archive Browser membutuhkan libarchive development package saat build.");
    }
    auto entries = ps2asset::archive::list_zip_entries(path.toStdString());
    archiveEntries_ = std::move(entries);
    archivePath_ = path;
    archiveTable_->setRowCount(static_cast<int>(archiveEntries_.size()));
    std::uint64_t totalBytes = 0;
    std::size_t directoryCount = 0;
    std::size_t fileCount = 0;
    for (std::size_t row = 0; row < archiveEntries_.size(); ++row) {
        const auto& entry = archiveEntries_[row];
        auto* name = new QTableWidgetItem(QString::fromUtf8(entry.path.c_str()));
        name->setData(Qt::UserRole, QString::fromUtf8(entry.path.c_str()));
        archiveTable_->setItem(static_cast<int>(row), 0, name);
        const QString type = entry.is_directory ? QStringLiteral("Direktori") :
            (entry.is_regular_file ? QStringLiteral("File") : QStringLiteral("Entri khusus"));
        archiveTable_->setItem(static_cast<int>(row), 1, new QTableWidgetItem(type));
        auto* sizeItem = new QTableWidgetItem(QString::number(static_cast<qulonglong>(entry.uncompressed_size)));
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        archiveTable_->setItem(static_cast<int>(row), 2, sizeItem);
        if (entry.is_directory) ++directoryCount;
        else if (entry.is_regular_file) {
            ++fileCount;
            if (entry.uncompressed_size > std::numeric_limits<std::uint64_t>::max() - totalBytes)
                totalBytes = std::numeric_limits<std::uint64_t>::max();
            else totalBytes += entry.uncompressed_size;
        }
    }
    archiveSummary_->setText(QStringLiteral("Arsip: %1\nEntri: %2  •  File: %3  •  Direktori: %4  •  Total ukuran file: %5 byte")
        .arg(QDir::toNativeSeparators(path))
        .arg(static_cast<qulonglong>(archiveEntries_.size()))
        .arg(static_cast<qulonglong>(fileCount))
        .arg(static_cast<qulonglong>(directoryCount))
        .arg(static_cast<qulonglong>(totalBytes)));
    archiveFilter_->clear();
    filterArchiveEntries();
}

void MainWindow::filterArchiveEntries() {
    if (archiveTable_ == nullptr || archiveFilter_ == nullptr) return;
    const QString filter = archiveFilter_->text().trimmed();
    for (int row = 0; row < archiveTable_->rowCount(); ++row) {
        auto* item = archiveTable_->item(row, 0);
        const bool match = item != nullptr && (filter.isEmpty() || item->text().contains(filter, Qt::CaseInsensitive));
        archiveTable_->setRowHidden(row, !match);
    }
}

void MainWindow::openSelectedArchiveTexture() {
    if (archiveTable_ == nullptr || archiveTable_->currentRow() < 0) {
        QMessageBox::information(this, QStringLiteral("Archive Browser"), QStringLiteral("Pilih satu entri terlebih dahulu."));
        return;
    }
    const auto* nameItem = archiveTable_->item(archiveTable_->currentRow(), 0);
    if (nameItem == nullptr) return;
    const QString entryName = nameItem->data(Qt::UserRole).toString();
    if (QFileInfo(entryName).suffix().compare(QStringLiteral("tm2"), Qt::CaseInsensitive) != 0) {
        QMessageBox::information(this, QStringLiteral("Bukan file TIM2"),
            QStringLiteral("Texture Workshop saat ini membuka entri .tm2 dari arsip. Untuk berkas lain, gunakan Binary Inspector pada hasil ekstraksi lokal jika diperlukan."));
        return;
    }
    const QString source = archivePath_;
    openPath(source);
    if (currentPath_ != source || moduleList_->currentRow() != TextureModule) return;
    for (int row = 0; row < textureList_->count(); ++row) {
        auto* item = textureList_->item(row);
        const int docIndex = item->data(kDocumentRole).toInt();
        if (docIndex >= 0 && static_cast<std::size_t>(docIndex) < documents_.size() &&
            documents_[static_cast<std::size_t>(docIndex)].name == entryName) {
            textureList_->setCurrentRow(row);
            return;
        }
    }
}

void MainWindow::inspectModelDialog() {
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Pilih model .mdl"), QString(),
        QStringLiteral("Model MDL (*.mdl);;Semua file (*.*)"));
    if (path.isEmpty()) return;

    try {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            throw std::runtime_error(
                QStringLiteral("Tidak dapat membuka berkas model: %1").arg(file.errorString()).toStdString());
        }
        if (file.size() < 0 || file.size() > kMaxBinaryInspectBytes) {
            throw std::runtime_error("Ukuran model melebihi batas read-only inspector sebesar 64 MiB.");
        }
        const auto size = static_cast<std::size_t>(file.size());
        std::vector<std::uint8_t> bytes(size);
        if (!bytes.empty()) {
            const qint64 requested = static_cast<qint64>(bytes.size());
            const qint64 read = file.read(reinterpret_cast<char*>(bytes.data()), requested);
            if (read != requested) {
                throw std::runtime_error("Pembacaan berkas model tidak lengkap.");
            }
        }
        const auto report = ps2asset::model::inspect_model_bytes(bytes, path.toStdString());
        modelReport_->setPlainText(QString::fromStdString(ps2asset::model::format_model_report(report)));
        selectModule(ModelModule);
        appendLog(QStringLiteral("[Model] Profiled %1 (%2 bytes; %3 marker types)")
            .arg(QDir::toNativeSeparators(path))
            .arg(static_cast<qulonglong>(report.file_size))
            .arg(static_cast<qulonglong>(report.markers.size())));
        statusBar()->showMessage(QStringLiteral("Profil MDL selesai. File hanya dibaca; tidak ada perubahan yang ditulis."), 7000);
    } catch (const std::exception& error) {
        QMessageBox::critical(this, QStringLiteral("Gagal memeriksa model"), safeExceptionText(error));
    }
}

void MainWindow::inspectBinaryDialog() {
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Pilih berkas untuk inspeksi binary"), currentPath_,
        QStringLiteral("Semua berkas (*.*);;Aset PS2 (*.tm2 *.irx *.elf *.pss *.map *.scp);;TIM2 (*.tm2);;ZIP (*.zip)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, QStringLiteral("Inspeksi gagal"), file.errorString());
        return;
    }
    const qint64 fileSize = file.size();
    if (fileSize < 0 || fileSize > kMaxBinaryInspectBytes) {
        QMessageBox::critical(this, QStringLiteral("Inspeksi dibatasi"),
            QStringLiteral("Binary Inspector menerima file sampai 64 MiB. Ukuran file: %1 byte.").arg(fileSize));
        return;
    }
    const QByteArray content = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        QMessageBox::critical(this, QStringLiteral("Inspeksi gagal"), file.errorString());
        return;
    }
    binaryBytes_.resize(static_cast<std::size_t>(content.size()));
    if (!content.isEmpty()) {
        std::copy(reinterpret_cast<const std::uint8_t*>(content.constData()),
                  reinterpret_cast<const std::uint8_t*>(content.constData()) + content.size(), binaryBytes_.begin());
    }
    binaryPath_ = path;

    const std::size_t prefixSize = std::min<std::size_t>(binaryBytes_.size(), 4096U);
    const std::vector<std::uint8_t> prefix(binaryBytes_.begin(), binaryBytes_.begin() + static_cast<std::ptrdiff_t>(prefixSize));
    const auto result = ps2asset::inspect_header(prefix, static_cast<std::uint64_t>(binaryBytes_.size()));
    QStringList lines;
    lines << QStringLiteral("File: %1").arg(QDir::toNativeSeparators(path));
    lines << QStringLiteral("Ukuran: %1 byte (%2 MiB)").arg(static_cast<qulonglong>(binaryBytes_.size()))
           .arg(static_cast<double>(binaryBytes_.size()) / (1024.0 * 1024.0), 0, 'f', 3);
    lines << QStringLiteral("Format terdeteksi: %1").arg(QString::fromStdString(result.format));
    for (const auto& detail : result.details) lines << QStringLiteral("• %1").arg(QString::fromStdString(detail));
    if (result.is_sl01 && result.sl01_length_checked) {
        lines << QStringLiteral("Validasi SL01: %1").arg(result.sl01_length_matches ? QStringLiteral("cocok") : QStringLiteral("tidak cocok"));
    }
    lines << QStringLiteral("Mode: read-only; tidak ada data yang diubah.");
    binaryInfo_->setText(lines.join(QLatin1Char('\n')));
    binarySearch_->clear();
    renderHexPreview();
    appendLog(QStringLiteral("[Binary] Inspected %1 (%2 bytes; %3)").arg(path)
        .arg(static_cast<qulonglong>(binaryBytes_.size())).arg(QString::fromStdString(result.format)));
    selectModule(BinaryModule);
    statusBar()->showMessage(QStringLiteral("Binary dimuat untuk inspeksi read-only: %1").arg(QFileInfo(path).fileName()), 6000);
}

void MainWindow::searchBinaryText() {
    if (binaryBytes_.empty()) {
        QMessageBox::information(this, QStringLiteral("Binary Inspector"), QStringLiteral("Pilih berkas terlebih dahulu."));
        return;
    }
    const QByteArray text = binarySearch_->text().toUtf8();
    if (text.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Pencarian"), QStringLiteral("Masukkan string ASCII yang ingin dicari."));
        return;
    }
    std::vector<std::uint8_t> needle;
    needle.reserve(static_cast<std::size_t>(text.size()));
    for (const char ch : text) needle.push_back(static_cast<std::uint8_t>(static_cast<unsigned char>(ch)));
    const auto found = std::search(binaryBytes_.begin(), binaryBytes_.end(), needle.begin(), needle.end());
    if (found == binaryBytes_.end()) {
        statusBar()->showMessage(QStringLiteral("String tidak ditemukan pada %1").arg(QFileInfo(binaryPath_).fileName()), 6000);
        QMessageBox::information(this, QStringLiteral("Pencarian"), QStringLiteral("String tidak ditemukan."));
        return;
    }
    const std::size_t offset = static_cast<std::size_t>(std::distance(binaryBytes_.begin(), found));
    statusBar()->showMessage(QStringLiteral("String ditemukan pada offset 0x%1 (%2 desimal).")
        .arg(static_cast<qulonglong>(offset), 0, 16).arg(static_cast<qulonglong>(offset)), 8000);
    if (offset < kMaxHexPreviewBytes) {
        const std::size_t rowOffset = offset - (offset % 16U);
        const QString rowPrefix = QStringLiteral("%1").arg(static_cast<qulonglong>(rowOffset), 8, 16, QLatin1Char('0')).toUpper();
        hexView_->moveCursor(QTextCursor::Start);
        (void)hexView_->find(rowPrefix);
    }
}

void MainWindow::renderHexPreview() {
    if (hexView_ == nullptr) return;
    if (binaryBytes_.empty()) {
        hexView_->setPlainText(QStringLiteral("Pilih berkas untuk melihat hex dump."));
        return;
    }
    const std::size_t count = std::min<std::size_t>(binaryBytes_.size(), kMaxHexPreviewBytes);
    std::ostringstream output;
    output << "OFFSET   " << "HEX BYTES                                         " << " ASCII\n";
    output << "-------- " << "------------------------------------------------- " << " ----------------\n";
    for (std::size_t offset = 0; offset < count; offset += 16U) {
        output << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << offset << "  ";
        std::string ascii;
        ascii.reserve(16U);
        for (std::size_t column = 0; column < 16U; ++column) {
            const std::size_t at = offset + column;
            if (at < count) {
                const unsigned int byte = binaryBytes_[at];
                output << std::setw(2) << byte << ' ';
                ascii.push_back(byte >= 32U && byte <= 126U ? static_cast<char>(byte) : '.');
            } else {
                output << "   ";
                ascii.push_back(' ');
            }
        }
        output << " |" << ascii << "|\n";
    }
    output << std::dec;
    if (count < binaryBytes_.size()) {
        output << "\n[Preview dipotong pada " << count << " byte; pencarian tetap memeriksa seluruh berkas yang dimuat.]\n";
    }
    hexView_->setPlainText(QString::fromStdString(output.str()));
    hexView_->moveCursor(QTextCursor::Start);
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
