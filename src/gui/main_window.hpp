#pragma once

#include "ps2asset/texture/tim2.hpp"

#include <QMainWindow>
#include <QString>
#include <QImage>
#include <memory>
#include <vector>

class QAction;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QResizeEvent;
class QPaintEvent;
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

private:
    struct TextureDocument {
        QString name;
        std::unique_ptr<ps2asset::texture::Tim2File> file;
        bool modified = false;
        bool savedAs = false;
    };

    void buildUi();
    void openDialog();
    void openPath(const QString& path);
    void openTim2(const QString& path);
    void openZip(const QString& path);
    void refreshList(int selectRow = 0);
    void selectionChanged(QListWidgetItem* item);
    void updateSelection(int documentIndex, int pictureIndex);
    void exportSelectedTexture();
    void importSelectedTexture();
    void saveSelectedTim2As();

    [[nodiscard]] QImage toQImage(const ps2asset::texture::RgbaImage& image) const;
    [[nodiscard]] ps2asset::texture::RgbaImage fromQImage(const QImage& image) const;
    [[nodiscard]] bool hasSelection() const noexcept;

    QListWidget* textureList_ = nullptr;
    ImagePreview* preview_ = nullptr;
    QLabel* details_ = nullptr;
    QLabel* sourceLabel_ = nullptr;
    QAction* importAction_ = nullptr;
    QAction* exportAction_ = nullptr;
    QAction* saveAction_ = nullptr;
    std::vector<TextureDocument> documents_;
    QString currentPath_;
    int selectedDocument_ = -1;
    int selectedPicture_ = -1;
};
