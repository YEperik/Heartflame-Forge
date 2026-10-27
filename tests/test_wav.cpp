#include "ps2asset/audio/wav.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
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
    std::cout << "All WAV metadata tests passed.\n";
    return 0;
}
