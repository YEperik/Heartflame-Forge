#include "ps2asset/texture/tim2.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void put_u16(std::vector<std::uint8_t>& b, std::size_t p, std::uint16_t v) {
    b[p] = static_cast<std::uint8_t>(v & 0xFFU);
    b[p + 1U] = static_cast<std::uint8_t>((v >> 8U) & 0xFFU);
}
void put_u32(std::vector<std::uint8_t>& b, std::size_t p, std::uint32_t v) {
    for (unsigned int i = 0; i < 4U; ++i) b[p + i] = static_cast<std::uint8_t>((v >> (8U * i)) & 0xFFU);
}
void put_u64(std::vector<std::uint8_t>& b, std::size_t p, std::uint64_t v) {
    put_u32(b, p, static_cast<std::uint32_t>(v & 0xFFFFFFFFULL));
    put_u32(b, p + 4U, static_cast<std::uint32_t>(v >> 32U));
}
std::vector<std::uint8_t> make_test_tm2() {
    constexpr std::size_t file_header = 16U, picture_header = 48U;
    constexpr std::size_t width = 4U, height = 2U, image_size = width * height, clut_size = 1024U;
    constexpr std::size_t picture_size = picture_header + image_size + clut_size;
    std::vector<std::uint8_t> b(file_header + picture_size, 0U);
    b[0] = 'T'; b[1] = 'I'; b[2] = 'M'; b[3] = '2'; b[4] = 4U; b[5] = 0U;
    put_u16(b, 6U, 1U);
    const std::size_t p = file_header;
    put_u32(b, p, static_cast<std::uint32_t>(picture_size));
    put_u32(b, p + 4U, static_cast<std::uint32_t>(clut_size));
    put_u32(b, p + 8U, static_cast<std::uint32_t>(image_size));
    put_u16(b, p + 12U, static_cast<std::uint16_t>(picture_header));
    put_u16(b, p + 14U, 256U);
    b[p + 16U] = 0U; b[p + 17U] = 1U; b[p + 18U] = 3U; b[p + 19U] = 5U;
    put_u16(b, p + 20U, static_cast<std::uint16_t>(width));
    put_u16(b, p + 22U, static_cast<std::uint16_t>(height));
    // Set GS PSM bits to 19 (PSMT8), keeping all other TEX0 fields zero.
    put_u64(b, p + 24U, static_cast<std::uint64_t>(19U) << 20U);
    const std::size_t image_off = p + picture_header;
    for (std::size_t i = 0; i < image_size; ++i) b[image_off + i] = static_cast<std::uint8_t>(i % 2U);
    const std::size_t clut_off = image_off + image_size;
    b[clut_off] = 240U; b[clut_off + 1U] = 20U; b[clut_off + 2U] = 10U; b[clut_off + 3U] = 128U;
    b[clut_off + 4U] = 10U; b[clut_off + 5U] = 220U; b[clut_off + 6U] = 30U; b[clut_off + 7U] = 0U;
    return b;
}
}

int main() {
    {
        auto file = ps2asset::texture::Tim2File::parse(make_test_tm2());
        assert(file.version() == 4U);
        assert(file.picture_count() == 1U);
        const auto& p = file.pictures().front();
        assert(p.width == 4U && p.height == 2U);
        assert(p.psm() == 19U);
        assert(p.supports_rgba8_palette_replace());
        auto image = file.decode_picture(0U);
        assert(image.valid() && image.width == 4U && image.height == 2U);
        assert(image.pixels[0] == 240U && image.pixels[1] == 20U && image.pixels[2] == 10U);
        assert(image.pixels[3] == 255U); // GS alpha 0x80 => fully opaque RGBA alpha.
        assert(image.pixels[7] == 0U);   // palette index 1 has alpha 0.

        ps2asset::texture::RgbaImage replacement;
        replacement.width = 4U; replacement.height = 2U;
        replacement.pixels = {
            255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,0,255,
            255,0,255,255, 0,255,255,255, 20,30,40,0, 250,250,250,128};
        const auto original_size = file.bytes().size();
        const auto original_header = std::vector<std::uint8_t>(file.bytes().begin(), file.bytes().begin() + 64);
        file.replace_picture_rgba8_palette(0U, replacement);
        assert(file.bytes().size() == original_size);
        assert(std::equal(original_header.begin(), original_header.begin() + 16, file.bytes().begin()));
        const auto roundtrip = ps2asset::texture::Tim2File::parse(file.bytes()).decode_picture(0U);
        assert(roundtrip.valid());
        assert(roundtrip.width == replacement.width && roundtrip.height == replacement.height);
        assert(roundtrip.pixels[0] == 255U && roundtrip.pixels[1] == 0U && roundtrip.pixels[2] == 0U);
        assert(roundtrip.pixels[4] == 0U && roundtrip.pixels[5] == 255U && roundtrip.pixels[6] == 0U);
    }
    {
        bool failed = false;
        try { (void)ps2asset::texture::Tim2File::parse({ 'T','I','M','2', 4U,0U,1U }); }
        catch (const std::exception&) { failed = true; }
        assert(failed);
    }
    {
        // Keep parsing/inspection permissive, but refuse to decode/replace an
        // indexed payload whose GS PSM contradicts the byte-indexed profile.
        auto bytes = make_test_tm2();
        put_u64(bytes, 16U + 24U, 0U); // PSMCT32/unknown for this indexed payload.
        auto file = ps2asset::texture::Tim2File::parse(std::move(bytes));
        assert(file.picture_count() == 1U);
        assert(file.pictures().front().psm() == 0U);
        assert(!file.pictures().front().supports_rgba8_palette_replace());
        bool decode_failed = false;
        try { (void)file.decode_picture(0U); }
        catch (const std::exception&) { decode_failed = true; }
        assert(decode_failed);
        ps2asset::texture::RgbaImage image;
        image.width = 4U; image.height = 2U; image.pixels.resize(4U * 2U * 4U, 0U);
        bool replace_failed = false;
        try { file.replace_picture_rgba8_palette(0U, image); }
        catch (const std::exception&) { replace_failed = true; }
        assert(replace_failed);
    }
    {
        auto file = ps2asset::texture::Tim2File::parse(make_test_tm2());
        ps2asset::texture::RgbaImage wrong_size;
        wrong_size.width = 2U; wrong_size.height = 2U; wrong_size.pixels.resize(16U, 0U);
        bool failed = false;
        try { file.replace_picture_rgba8_palette(0U, wrong_size); }
        catch (const std::exception&) { failed = true; }
        assert(failed);
    }
    std::cout << "TIM2 parser/decode/replace tests passed.\n";
    return 0;
}
