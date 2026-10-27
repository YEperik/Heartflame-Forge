#include "ps2asset/audio/wav.hpp"
#include "ps2asset/inspect.hpp"
#include "ps2asset/model/containers.hpp"
#include "ps2asset/model/inspect.hpp"
#include "ps2asset/text/scan.hpp"
#include "ps2asset/texture/tim2.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void put16(std::vector<std::uint8_t>& b, std::size_t p, std::uint16_t x) {
    b[p] = static_cast<std::uint8_t>(x & 0xffU);
    b[p + 1U] = static_cast<std::uint8_t>((x >> 8U) & 0xffU);
}
void put32(std::vector<std::uint8_t>& b, std::size_t p, std::uint32_t x) {
    for (unsigned int i = 0U; i < 4U; ++i) b[p + i] = static_cast<std::uint8_t>((x >> (8U * i)) & 0xffU);
}
std::vector<std::uint8_t> valid_tim2() {
    constexpr std::size_t header = 16U, pic = 48U, pixels = 16U, clut = 1024U;
    std::vector<std::uint8_t> b(header + pic + pixels + clut, 0U);
    b[0] = 'T'; b[1] = 'I'; b[2] = 'M'; b[3] = '2'; b[4] = 4U;
    put16(b, 6U, 1U);
    put32(b, header, static_cast<std::uint32_t>(pic + pixels + clut));
    put32(b, header + 4U, clut); put32(b, header + 8U, pixels);
    put16(b, header + 12U, pic); put16(b, header + 14U, 256U);
    b[header + 17U] = 1U; b[header + 18U] = 3U; b[header + 19U] = 5U;
    put16(b, header + 20U, 4U); put16(b, header + 22U, 4U);
    put32(b, header + 24U, 19U << 20U);
    return b;
}
void exercise(const std::vector<std::uint8_t>& b) {
    (void)ps2asset::inspect_header(b, b.size());
    (void)ps2asset::audio::inspect_wav_bytes(b);
    (void)ps2asset::text::scan_printable_strings(b, 2U, 32U);
    (void)ps2asset::model::scan_embedded_sl01_blocks(b);
    (void)ps2asset::model::inspect_model_bytes(b, "fuzz.mdl");
    try {
        const auto file = ps2asset::texture::Tim2File::parse(b);
        for (std::size_t i = 0; i < file.pictures().size(); ++i) {
            try { (void)file.decode_picture(i); } catch (const std::exception&) { /* unsupported profile is expected */ }
        }
    } catch (const std::exception&) {
        // A malformed binary is an expected parser outcome; crashes/UB are not.
    }
}
}

int main() {
    std::mt19937 rng(0x50533241U);
    for (std::size_t iteration = 0; iteration < 10000U; ++iteration) {
        const std::size_t size = static_cast<std::size_t>(rng() % 1537U);
        std::vector<std::uint8_t> bytes(size);
        for (auto& byte : bytes) byte = static_cast<std::uint8_t>(rng() & 0xffU);
        if (iteration % 7U == 0U && bytes.size() >= 12U) {
            bytes[0] = 'S'; bytes[1] = 'L'; bytes[2] = '0'; bytes[3] = '1';
            put32(bytes, 8U, static_cast<std::uint32_t>(rng()));
        }
        if (iteration % 11U == 0U && bytes.size() >= 12U) {
            bytes[0] = 'R'; bytes[1] = 'I'; bytes[2] = 'F'; bytes[3] = 'F';
            bytes[8] = 'W'; bytes[9] = 'A'; bytes[10] = 'V'; bytes[11] = 'E';
            put32(bytes, 4U, static_cast<std::uint32_t>(bytes.size() - 8U));
        }
        exercise(bytes);
    }

    const auto seed = valid_tim2();
    exercise(seed);
    for (std::size_t iteration = 0; iteration < 2000U; ++iteration) {
        auto bytes = seed;
        const std::size_t mutations = 1U + static_cast<std::size_t>(rng() % 4U);
        for (std::size_t i = 0; i < mutations; ++i) {
            const auto pos = static_cast<std::size_t>(rng() % bytes.size());
            bytes[pos] = static_cast<std::uint8_t>(rng() & 0xffU);
        }
        exercise(bytes);
    }
    std::cout << "Parser malformed-input smoke test passed (10000 random buffers + 2000 mutated TIM2 seeds).\n";
    return 0;
}
