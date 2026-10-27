#include "text_page.hpp"
#include "ps2asset/text/scan.hpp"
namespace ps2asset::gui {
TextPage::TextPage(QWidget* parent)
    : FeaturePageBase(
        QStringLiteral("Text & Script Inspector"),
        QStringLiteral("Cari string printable ASCII dan string UTF-16LE yang karakter-nya berada di rentang ASCII, tampilkan offset, dan gunakan CLI untuk ekspor TSV. Hasil bukan decoder script."),
        QStringLiteral("Data dan script (*.txt *.dat *.bin *.msg *.scr *.tbl);;Semua file (*.*)"),
        [](const QString& path, const std::vector<std::uint8_t>& bytes) {
            const auto result = ps2asset::text::scan_printable_strings(bytes, 4U, 10000U);
            return QStringLiteral("File: %1\n%2")
                .arg(path)
                .arg(QString::fromStdString(ps2asset::text::format_string_scan_report(result)));
        }, parent) {}
}
