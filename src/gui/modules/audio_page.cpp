#include "audio_page.hpp"
#include "ps2asset/audio/wav.hpp"
#include "ps2asset/inspect.hpp"
#include <algorithm>

namespace ps2asset::gui {
AudioPage::AudioPage(QWidget* parent)
    : FeaturePageBase(
        QStringLiteral("Audio Inspector"),
        QStringLiteral("Inspeksi metadata RIFF/WAVE serta signature audio/resource. Modul ini belum memutar atau mendekode codec proprietary SDPK/ADSC."),
        QStringLiteral("Audio dan resource (*.wav *.WAV *.adsc *.ADSC *.sdpk *.SDPK *.bin);;Semua file (*.*)"),
        [](const QString& path, const std::vector<std::uint8_t>& bytes) {
            const auto wav = ps2asset::audio::inspect_wav_bytes(bytes);
            if (wav.state != ps2asset::audio::WavState::NotWave) {
                return QStringLiteral("File: %1\nSize: %2 bytes\n%3")
                    .arg(path).arg(static_cast<qulonglong>(bytes.size()))
                    .arg(QString::fromStdString(ps2asset::audio::format_wav_report(wav)));
            }
            return FeaturePageBase::genericHeaderReport(path, bytes) +
                QStringLiteral("\nInformasi codec: belum didekode. Signature proprietary hanya identifikasi awal.\n");
        }, parent) {}
}
