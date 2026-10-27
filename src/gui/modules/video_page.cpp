#include "video_page.hpp"
namespace ps2asset::gui {
VideoPage::VideoPage(QWidget* parent)
    : FeaturePageBase(
        QStringLiteral("Video / Stream Inspector"),
        QStringLiteral("Identifikasi read-only untuk stream MPEG/PSS-like dan header biner. Tidak semua signature berarti stream dapat diputar; parser blok PS2 belum terverifikasi."),
        QStringLiteral("Video dan stream (*.pss *.str *.m2v *.mpg *.mpeg *.bin);;Semua file (*.*)"),
        [](const QString& path, const std::vector<std::uint8_t>& bytes) {
            return FeaturePageBase::genericHeaderReport(path, bytes) +
                QStringLiteral("\nStatus decoder: tidak ada. Demux, codec, timestamp, dan struktur blok belum dianalisis.\n");
        }, parent) {}
}
