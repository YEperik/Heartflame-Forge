#include "feature_page_base.hpp"

#include "ps2asset/inspect.hpp"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace ps2asset::gui {
namespace {
constexpr qint64 kMaxFeatureFileBytes = 64LL * 1024LL * 1024LL;
}

FeaturePageBase::FeaturePageBase(const QString& title,
                                 const QString& summary,
                                 const QString& fileFilter,
                                 Inspector inspector,
                                 QWidget* parent)
    : QWidget(parent), fileFilter_(fileFilter), inspector_(std::move(inspector)) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(12);

    auto* heading = new QLabel(title, this);
    heading->setStyleSheet(QStringLiteral("font-size: 23px; font-weight: 700; color: #233246;"));
    auto* desc = new QLabel(summary, this);
    desc->setWordWrap(true);
    desc->setStyleSheet(QStringLiteral("color: #667487; font-size: 13px;"));
    auto* openButton = new QPushButton(QStringLiteral("Pilih berkas untuk inspeksi…"), this);
    openButton->setMinimumHeight(36);
    connect(openButton, &QPushButton::clicked, this, [this]() { chooseFile(); });

    fileLabel_ = new QLabel(QStringLiteral("Belum ada berkas dipilih."), this);
    fileLabel_->setWordWrap(true);
    fileLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    fileLabel_->setStyleSheet(QStringLiteral("background: white; border: 1px solid #dfe5ed; border-radius: 5px; padding: 8px; color: #2c3949;"));
    statusLabel_ = new QLabel(QStringLiteral("READ-ONLY · File sumber tidak diubah"), this);
    statusLabel_->setWordWrap(true);
    statusLabel_->setStyleSheet(QStringLiteral("color: #2f6542; background: #eaf6ee; border: 1px solid #b7dec3; border-radius: 5px; padding: 7px 9px;"));
    report_ = new QPlainTextEdit(this);
    report_->setReadOnly(true);
    report_->setLineWrapMode(QPlainTextEdit::NoWrap);
    report_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    report_->setStyleSheet(QStringLiteral("QPlainTextEdit { background: #151a22; color: #d8e3f0; border: 1px solid #2a3442; padding: 10px; }"));
    report_->setPlaceholderText(QStringLiteral("Pilih file untuk melihat hasil inspeksi."));

    layout->addWidget(heading);
    layout->addWidget(desc);
    layout->addWidget(openButton, 0, Qt::AlignLeft);
    layout->addWidget(fileLabel_);
    layout->addWidget(statusLabel_);
    layout->addWidget(report_, 1);
}

void FeaturePageBase::setStatus(const QString& message) {
    if (statusLabel_ != nullptr) statusLabel_->setText(message);
}

void FeaturePageBase::setReport(const QString& text) {
    if (report_ != nullptr) report_->setPlainText(text);
}

QString FeaturePageBase::genericHeaderReport(const QString& path, const std::vector<std::uint8_t>& bytes) {
    const std::size_t prefixSize = std::min<std::size_t>(bytes.size(), 4096U);
    std::vector<std::uint8_t> prefix(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(prefixSize));
    const auto result = ps2asset::inspect_header(prefix, static_cast<std::uint64_t>(bytes.size()));
    QString report = QStringLiteral("File: %1\nSize: %2 bytes\nDetected: %3\n")
        .arg(QFileInfo(path).fileName()).arg(static_cast<qulonglong>(bytes.size()))
        .arg(QString::fromStdString(result.format));
    for (const auto& detail : result.details) report += QStringLiteral("  %1\n").arg(QString::fromStdString(detail));
    return report;
}

void FeaturePageBase::chooseFile() {
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Pilih berkas"), QString(), fileFilter_);
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setStatus(QStringLiteral("GAGAL · %1").arg(file.errorString()));
        setReport(QString());
        return;
    }
    if (file.size() < 0 || file.size() > kMaxFeatureFileBytes) {
        setStatus(QStringLiteral("DITOLAK · Ukuran file melebihi batas inspeksi 64 MiB."));
        setReport(QString());
        return;
    }
    const QByteArray data = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        setStatus(QStringLiteral("GAGAL · Tidak dapat membaca file: %1").arg(file.errorString()));
        setReport(QString());
        return;
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(data.size()));
    if (!data.isEmpty()) {
        const auto* begin = reinterpret_cast<const std::uint8_t*>(data.constData());
        std::copy(begin, begin + data.size(), bytes.begin());
    }
    fileLabel_->setText(QStringLiteral("%1  ·  %2 bytes").arg(QFileInfo(path).fileName()).arg(static_cast<qulonglong>(bytes.size())));
    try {
        setReport(inspector_ ? inspector_(path, bytes) : genericHeaderReport(path, bytes));
        setStatus(QStringLiteral("SELESAI · inspeksi read-only; sumber tidak diubah."));
    } catch (const std::exception& ex) {
        setStatus(QStringLiteral("GAGAL INSPEKSI · %1").arg(QString::fromUtf8(ex.what())));
        setReport(QString());
    } catch (...) {
        setStatus(QStringLiteral("GAGAL INSPEKSI · exception tidak dikenal."));
        setReport(QString());
    }
}

}  // namespace ps2asset::gui
