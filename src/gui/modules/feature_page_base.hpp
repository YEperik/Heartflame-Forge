#pragma once

#include <QWidget>
#include <QString>
#include <functional>
#include <cstdint>
#include <vector>

class QLabel;
class QPlainTextEdit;
class QPushButton;

namespace ps2asset::gui {

class FeaturePageBase : public QWidget {
public:
    using Inspector = std::function<QString(const QString&, const std::vector<std::uint8_t>&)>;

    FeaturePageBase(const QString& title,
                    const QString& summary,
                    const QString& fileFilter,
                    Inspector inspector,
                    QWidget* parent = nullptr);

protected:
    void setStatus(const QString& message);
    void setReport(const QString& text);
    static QString genericHeaderReport(const QString& path, const std::vector<std::uint8_t>& bytes);

private:
    void chooseFile();

    QString fileFilter_;
    Inspector inspector_;
    QLabel* fileLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QPlainTextEdit* report_ = nullptr;
};

}  // namespace ps2asset::gui
