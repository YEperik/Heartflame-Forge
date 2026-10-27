#include "animation_page.hpp"
#include "ps2asset/model/inspect.hpp"
namespace ps2asset::gui {
AnimationPage::AnimationPage(QWidget* parent)
    : FeaturePageBase(
        QStringLiteral("Animation Inspector"),
        QStringLiteral("Workspace terpisah untuk riset struktur animasi. Yang tersedia saat ini hanya profiling marker/container read-only; keyframe dan skeleton belum didekode."),
        QStringLiteral("Kandidat model/animasi (*.mdl *.anm *.ani *.bin);;Semua file (*.*)"),
        [](const QString& path, const std::vector<std::uint8_t>& bytes) {
            return QString::fromStdString(ps2asset::model::format_model_report(
                ps2asset::model::inspect_model_bytes(bytes, path.toStdString()))) +
                QStringLiteral("\nANIMATION NOTE: hasil marker tidak membuktikan bahwa file berisi animasi.\n");
        }, parent) {}
}
