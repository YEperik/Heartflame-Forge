#include "ps2asset/audio/wav.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <algorithm>
#include <string>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {
void put16(std::vector<std::uint8_t>& b, std::size_t at, std::uint16_t v) {
    b[at] = static_cast<std::uint8_t>(v & 0xFFU);
    b[at + 1U] = static_cast<std::uint8_t>((v >> 8U) & 0xFFU);
}
void put32(std::vector<std::uint8_t>& b, std::size_t at, std::uint32_t v) {
    b[at] = static_cast<std::uint8_t>(v & 0xFFU);
    b[at + 1U] = static_cast<std::uint8_t>((v >> 8U) & 0xFFU);
    b[at + 2U] = static_cast<std::uint8_t>((v >> 16U) & 0xFFU);
    b[at + 3U] = static_cast<std::uint8_t>((v >> 24U) & 0xFFU);
}
void put4(std::vector<std::uint8_t>& b, std::size_t at, const char* s) {
    for (std::size_t i = 0; i < 4U; ++i) b[at + i] = static_cast<std::uint8_t>(s[i]);
}
std::vector<std::uint8_t> pcm_fixture() {
    std::vector<std::uint8_t> b(44U, 0U);
    put4(b, 0U, "RIFF"); put32(b, 4U, 36U); put4(b, 8U, "WAVE");
    put4(b, 12U, "fmt "); put32(b, 16U, 16U);
    put16(b, 20U, 1U); put16(b, 22U, 2U); put32(b, 24U, 44100U);
    put32(b, 28U, 176400U); put16(b, 32U, 4U); put16(b, 34U, 16U);
    put4(b, 36U, "data"); put32(b, 40U, 0U);
    return b;
}
void refresh_riff_size(std::vector<std::uint8_t>& b) {
    put32(b, 4U, static_cast<std::uint32_t>(b.size() - 8U));
}
std::vector<std::uint8_t> extensible_pcm_fixture() {
    std::vector<std::uint8_t> b(68U, 0U);
    put4(b, 0U, "RIFF"); put32(b, 4U, 60U); put4(b, 8U, "WAVE");
    put4(b, 12U, "fmt "); put32(b, 16U, 40U);
    put16(b, 20U, 0xFFFEU); put16(b, 22U, 2U); put32(b, 24U, 44100U);
    put32(b, 28U, 176400U); put16(b, 32U, 4U); put16(b, 34U, 16U);
    put16(b, 36U, 22U); put16(b, 38U, 16U); put32(b, 40U, 3U);
    put32(b, 44U, 1U); put16(b, 48U, 0U); put16(b, 50U, 0x0010U);
    b[52U] = 0x80U; b[53U] = 0x00U; b[54U] = 0x00U; b[55U] = 0xAAU;
    b[56U] = 0x00U; b[57U] = 0x38U; b[58U] = 0x9BU; b[59U] = 0x71U;
    put4(b, 60U, "data"); put32(b, 64U, 0U);
    return b;
}
}

int main() {
    {
        const auto b = pcm_fixture();
        const auto r = ps2asset::audio::inspect_wav_bytes(b);
        assert(r.state == ps2asset::audio::WavState::ValidMetadata);
        assert(r.has_format_chunk && r.has_data_chunk);
        assert(r.format_tag == 1U && r.channels == 2U && r.sample_rate == 44100U);
        assert(r.byte_rate == 176400U && r.block_align == 4U && r.bits_per_sample == 16U);
        assert(r.data_bytes == 0U && r.chunks.size() == 2U);
        assert(ps2asset::audio::format_wav_report(r).find("PCM") != std::string::npos);
    }
    {
        auto b = extensible_pcm_fixture();
        const auto r = ps2asset::audio::inspect_wav_bytes(b);
        assert(r.state == ps2asset::audio::WavState::ValidMetadata);
        assert(r.format_tag == 0xFFFEU && r.channels == 2U && r.block_align == 4U);
    }
    {
        auto b = extensible_pcm_fixture();
        put32(b, 44U, 0x00010001U); // low 16 bits equal PCM, but the full subtype is not PCM
        put32(b, 28U, 88200U);
        put16(b, 32U, 2U);
        const auto r = ps2asset::audio::inspect_wav_bytes(b);
        assert(r.state == ps2asset::audio::WavState::ValidMetadata);
    }
    {
        auto b = pcm_fixture();
        put16(b, 32U, 2U);
        const auto r = ps2asset::audio::inspect_wav_bytes(b);
        assert(r.state == ps2asset::audio::WavState::Malformed);
        assert(r.error.find("block_align") != std::string::npos);
    }
    {
        auto b = pcm_fixture();
        put32(b, 28U, 999U);
        const auto r = ps2asset::audio::inspect_wav_bytes(b);
        assert(r.state == ps2asset::audio::WavState::Malformed);
        assert(r.error.find("byte_rate") != std::string::npos);
    }
    {
        auto b = pcm_fixture();
        put16(b, 20U, 0xFFFEU);
        const auto r = ps2asset::audio::inspect_wav_bytes(b);
        assert(r.state == ps2asset::audio::WavState::Malformed);
        assert(r.error.find("40 bytes") != std::string::npos);
    }
    {
        auto b = pcm_fixture();
        const auto fmt = std::vector<std::uint8_t>(b.begin() + 12, b.begin() + 36);
        b.insert(b.end(), fmt.begin(), fmt.end());
        refresh_riff_size(b);
        const auto r = ps2asset::audio::inspect_wav_bytes(b);
        assert(r.state == ps2asset::audio::WavState::Malformed);
        assert(r.error.find("duplicate fmt") != std::string::npos);
    }
    {
        auto b = pcm_fixture();
        b[36U] = 'd'; b[37U] = 'a'; b[38U] = 't'; b[39U] = 'a';
        put32(b, 40U, 2U);
        b.resize(46U, 0U);
        b[44U] = 0xAAU; b[45U] = 0xBBU;
        refresh_riff_size(b);
        const auto r = ps2asset::audio::inspect_wav_bytes(b);
        assert(r.state == ps2asset::audio::WavState::Malformed);
        assert(r.error.find("sample frames") != std::string::npos);
    }
    {
        auto b = pcm_fixture();
        b.resize(12U, 0U);
        put4(b, 0U, "RIFF"); put4(b, 8U, "WAVE");
        const std::size_t count = 65537U;
        b.resize(12U + count * 8U + 32U, 0U);
        std::size_t off = 12U;
        for (std::size_t i = 0; i < count; ++i, off += 8U) {
            put4(b, off, "JUNK"); put32(b, off + 4U, 0U);
        }
        put4(b, off, "fmt "); put32(b, off + 4U, 16U);
        put16(b, off + 8U, 1U); put16(b, off + 10U, 2U); put32(b, off + 12U, 44100U);
        put32(b, off + 16U, 176400U); put16(b, off + 20U, 4U); put16(b, off + 22U, 16U);
        off += 24U; put4(b, off, "data"); put32(b, off + 4U, 0U);
        refresh_riff_size(b);
        const auto r = ps2asset::audio::inspect_wav_bytes(b);
        assert(r.state == ps2asset::audio::WavState::Malformed);
        assert(r.error.find("chunk count exceeds") != std::string::npos);
    }
    {
        auto b = pcm_fixture();
        put32(b, 16U, 0xFFFFFFF0U);
        const auto r = ps2asset::audio::inspect_wav_bytes(b);
        assert(r.state == ps2asset::audio::WavState::Malformed);
        assert(!r.error.empty());
    }
    {
        std::vector<std::uint8_t> b{'R','I','F','F',0,0,0,0,'N','O','P','E'};
        assert(ps2asset::audio::inspect_wav_bytes(b).state == ps2asset::audio::WavState::NotWave);
    }
    std::cout << "All WAV metadata tests passed (including malformed and resource-limit fixtures).\n";
    return 0;
}
