#pragma once

#include "ps2asset/texture/tim2.hpp"
#include "ps2asset/archive/zip_reader.hpp"
#include "plugin_manager.hpp"

#include <QMainWindow>
#include <QImage>
#include <QString>
#include <QStringList>
#include <cstdint>
#include <memory>
#include <vector>

class QAction;
class QLabel;
class QLineEdit;
class QDockWidget;
class QPlainTextEdit;
class QCloseEvent;
class QListWidget;
class QListWidgetItem;
class QPaintEvent;
class QStackedWidget;
class QTableWidget;
class QTextEdit;
class QWidget;

class ImagePreview final : public QWidget {
public:
    explicit ImagePreview(QWidget* parent = nullptr);
    void setImage(const QImage& image);
    void setMessage(const QString& message);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage image_;
    QString message_ = QStringLiteral("Pilih tekstur untuk melihat pratinjau");
};

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    struct TextureDocument {
        QString name;
        std::unique_ptr<ps2asset::texture::Tim2File> file;
        bool modified = false;
        bool savedAs = false;
    };

    enum ModuleRow {
        DashboardModule = 0,
        TextureModule,
        ArchiveModule,
        BinaryModule,
        ModelModule,
        AudioModule,
        ScriptModule,
        BuiltInModuleCount
    };

    void buildUi();
    QWidget* buildDashboardPage();
    QWidget* buildTexturePage();
    QWidget* buildArchiveBrowserPage();
    QWidget* buildBinaryInspectorPage();
    QWidget* buildPlaceholderPage(const QString& title, const QString& summary,
                                  const QStringList& plannedFeatures);
    void selectModule(int row);
    void loadExtensionModules();
    void updateDashboardSummary();
    void appendLog(const QString& message);

    void openDialog();
    void openPath(const QString& path);
    [[nodiscard]] bool confirmDiscardUnsavedChanges();
    void openTim2(const QString& path);
    void openZip(const QString& path);
    void refreshList(int selectRow = 0);
    void selectionChanged(QListWidgetItem* item);
    void updateSelection(int documentIndex, int pictureIndex);
    void exportSelectedTexture();
    void importSelectedTexture();
    void saveSelectedTim2As();

    void openArchiveDialog();
    void loadArchiveListing(const QString& path);
    void filterArchiveEntries();
    void openSelectedArchiveTexture();
    void inspectBinaryDialog();
    void searchBinaryText();
    void renderHexPreview();

    [[nodiscard]] QImage toQImage(const ps2asset::texture::RgbaImage& image) const;
    [[nodiscard]] ps2asset::texture::RgbaImage fromQImage(const QImage& image) const;
    [[nodiscard]] bool hasSelection() const noexcept;

    QListWidget* moduleList_ = nullptr;
    QStackedWidget* moduleStack_ = nullptr;
    QListWidget* textureList_ = nullptr;
    ImagePreview* preview_ = nullptr;
    QLabel* details_ = nullptr;
    QLabel* sourceLabel_ = nullptr;
    QLabel* dashboardSummary_ = nullptr;
    QLabel* binaryInfo_ = nullptr;
    QLabel* archiveSummary_ = nullptr;
    QDockWidget* logDock_ = nullptr;
    QPlainTextEdit* logView_ = nullptr;
    QTableWidget* archiveTable_ = nullptr;
    QLineEdit* archiveFilter_ = nullptr;
    QTextEdit* hexView_ = nullptr;
    QLineEdit* binarySearch_ = nullptr;
    QAction* importAction_ = nullptr;
    QAction* exportAction_ = nullptr;
    QAction* saveAction_ = nullptr;
    std::vector<TextureDocument> documents_;
    std::vector<std::uint8_t> binaryBytes_;
    std::vector<ps2asset::archive::ArchiveEntryInfo> archiveEntries_;
    QString archivePath_;
    QString binaryPath_;
    QString currentPath_;
    int selectedDocument_ = -1;
    int selectedPicture_ = -1;
    ps2asset::workbench::PluginManager pluginManager_;
};
