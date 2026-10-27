#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ps2asset::audio {

enum class WavState {
    NotWave,
    ValidMetadata,
    Malformed
};

struct WavChunkInfo {
    std::string id;
    std::uint64_t offset = 0;
    std::uint32_t payload_size = 0;
};

struct WavInspection {
    WavState state = WavState::NotWave;
    std::string error;
    std::uint32_t riff_declared_size = 0;
    std::uint16_t format_tag = 0;
    std::uint16_t channels = 0;
    std::uint32_t sample_rate = 0;
    std::uint32_t byte_rate = 0;
    std::uint16_t block_align = 0;
    std::uint16_t bits_per_sample = 0;
    std::uint64_t data_bytes = 0;
    std::uint64_t data_offset = 0;
    bool has_format_chunk = false;
    bool has_data_chunk = false;
    std::vector<WavChunkInfo> chunks;
};

// Parses only RIFF/WAVE chunk metadata. It never decodes or plays audio.
// The caller must cap input size before allocating the vector.
WavInspection inspect_wav_bytes(const std::vector<std::uint8_t>& bytes);
std::string format_wav_report(const WavInspection& result);

}  // namespace ps2asset::audio
