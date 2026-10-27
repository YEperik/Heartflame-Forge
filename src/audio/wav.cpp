#include "ps2asset/audio/wav.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace ps2asset::audio {
namespace {

bool starts(const std::vector<std::uint8_t>& bytes, std::size_t offset, const char* tag) {
    return offset <= bytes.size() && bytes.size() - offset >= 4U &&
           bytes[offset] == static_cast<std::uint8_t>(tag[0]) &&
           bytes[offset + 1U] == static_cast<std::uint8_t>(tag[1]) &&
           bytes[offset + 2U] == static_cast<std::uint8_t>(tag[2]) &&
           bytes[offset + 3U] == static_cast<std::uint8_t>(tag[3]);
}

std::uint16_t u16le(const std::vector<std::uint8_t>& b, std::size_t off) {
    return static_cast<std::uint16_t>(b[off]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(b[off + 1U]) << 8U);
}

std::uint32_t u32le(const std::vector<std::uint8_t>& b, std::size_t off) {
    return static_cast<std::uint32_t>(b[off]) |
           (static_cast<std::uint32_t>(b[off + 1U]) << 8U) |
           (static_cast<std::uint32_t>(b[off + 2U]) << 16U) |
           (static_cast<std::uint32_t>(b[off + 3U]) << 24U);
}

std::string fourcc(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    std::string result;
    result.reserve(4U);
    for (std::size_t i = 0; i < 4U; ++i) {
        const auto c = bytes[offset + i];
        result.push_back(c >= 0x20U && c <= 0x7EU ? static_cast<char>(c) : '?');
    }
    return result;
}

const char* format_name(std::uint16_t tag) {
    switch (tag) {
    case 1U: return "PCM";
    case 3U: return "IEEE float";
    case 0xFFFEU: return "WAVE_FORMAT_EXTENSIBLE";
    default: return "unknown/compressed";
    }
}

}  // namespace

WavInspection inspect_wav_bytes(const std::vector<std::uint8_t>& bytes) {
    WavInspection result;
    if (bytes.size() < 12U || !starts(bytes, 0U, "RIFF") || !starts(bytes, 8U, "WAVE")) {
        return result;
    }

    result.riff_declared_size = u32le(bytes, 4U);
    result.state = WavState::Malformed;
    const std::uint64_t riff_end64 = static_cast<std::uint64_t>(result.riff_declared_size) + 8U;
    if (riff_end64 < 12U || riff_end64 > static_cast<std::uint64_t>(bytes.size())) {
        result.error = "RIFF declared size exceeds available bytes or is too short";
        return result;
    }
    const std::size_t riff_end = static_cast<std::size_t>(riff_end64);
    std::size_t offset = 12U;
    while (offset < riff_end) {
        if (riff_end - offset < 8U) {
            result.error = "truncated chunk header at offset " + std::to_string(offset);
            return result;
        }
        const std::string id = fourcc(bytes, offset);
        const std::uint32_t payload_size = u32le(bytes, offset + 4U);
        const std::size_t payload_offset = offset + 8U;
        const std::uint64_t payload_end64 = static_cast<std::uint64_t>(payload_offset) + payload_size;
        if (payload_end64 > riff_end64 || payload_end64 > static_cast<std::uint64_t>(bytes.size())) {
            result.error = "chunk " + id + " payload exceeds RIFF bounds";
            return result;
        }
        result.chunks.push_back(WavChunkInfo{id, static_cast<std::uint64_t>(offset), payload_size});

        if (id == "fmt ") {
            if (payload_size < 16U) {
                result.error = "fmt chunk is shorter than the 16-byte base WAVEFORMAT structure";
                return result;
            }
            result.format_tag = u16le(bytes, payload_offset);
            result.channels = u16le(bytes, payload_offset + 2U);
            result.sample_rate = u32le(bytes, payload_offset + 4U);
            result.byte_rate = u32le(bytes, payload_offset + 8U);
            result.block_align = u16le(bytes, payload_offset + 12U);
            result.bits_per_sample = u16le(bytes, payload_offset + 14U);
            if (result.channels == 0U || result.sample_rate == 0U || result.block_align == 0U) {
                result.error = "fmt chunk contains zero channel/sample-rate/block-align field";
                return result;
            }
            result.has_format_chunk = true;
        } else if (id == "data") {
            result.has_data_chunk = true;
            result.data_bytes += payload_size;
            if (result.data_offset == 0U) result.data_offset = static_cast<std::uint64_t>(payload_offset);
        }

        const std::uint64_t next64 = payload_end64 + (payload_size & 1U);
        if (next64 > riff_end64) {
            result.error = "chunk padding byte exceeds RIFF bounds";
            return result;
        }
        offset = static_cast<std::size_t>(next64);
    }

    if (offset != riff_end) {
        result.error = "RIFF chunks do not end on the declared RIFF boundary";
        return result;
    }
    if (!result.has_format_chunk || !result.has_data_chunk) {
        result.error = std::string("missing required ") +
                       (!result.has_format_chunk ? "fmt " : "data") + " chunk";
        return result;
    }
    result.state = WavState::ValidMetadata;
    return result;
}

std::string format_wav_report(const WavInspection& r) {
    std::ostringstream out;
    out << "WAV metadata inspection (read-only)\n";
    if (r.state == WavState::NotWave) {
        out << "status: not a RIFF/WAVE file\n";
        return out.str();
    }
    out << "status: " << (r.state == WavState::ValidMetadata ? "valid metadata" : "malformed") << '\n';
    out << "RIFF declared bytes: " << (static_cast<std::uint64_t>(r.riff_declared_size) + 8U) << '\n';
    if (r.has_format_chunk) {
        out << "format tag: 0x" << std::hex << std::uppercase << r.format_tag << std::dec
            << " (" << format_name(r.format_tag) << ")\n";
        out << "channels: " << r.channels << '\n';
        out << "sample rate: " << r.sample_rate << " Hz\n";
        out << "byte rate: " << r.byte_rate << '\n';
        out << "block align: " << r.block_align << '\n';
        out << "bits per sample: " << r.bits_per_sample << '\n';
    }
    out << "data bytes: " << r.data_bytes << '\n';
    if (r.has_data_chunk) out << "first data payload offset: 0x" << std::hex << r.data_offset << std::dec << '\n';
    out << "chunks: " << r.chunks.size() << '\n';
    for (const auto& chunk : r.chunks) {
        out << "  " << chunk.id << " offset=0x" << std::hex << chunk.offset << std::dec
            << " payload=" << chunk.payload_size << '\n';
    }
    if (!r.error.empty()) out << "error: " << r.error << '\n';
    out << "note: metadata only; no sample decoding, playback, or editing performed\n";
    return out.str();
}

}  // namespace ps2asset::audio
