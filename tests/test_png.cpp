#include "ps2asset/image/png.hpp"
#include "ps2asset/texture/tim2.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace {
void put_u16(std::vector<std::uint8_t>& bytes, std::size_t at, std::uint16_t value) {
    bytes[at] = static_cast<std::uint8_t>(value & 0xFFU);
    bytes[at + 1U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
}
void put_u32(std::vector<std::uint8_t>& bytes, std::size_t at, std::uint32_t value) {
    for (unsigned int i = 0; i < 4U; ++i) bytes[at + i] = static_cast<std::uint8_t>((value >> (8U * i)) & 0xFFU);
}

std::vector<std::uint8_t> fixture_tm2(std::uint16_t width = 4U, std::uint16_t height = 2U) {
    constexpr std::size_t file_header = 16U;
    constexpr std::size_t picture_header = 48U;
    const std::size_t image_size = static_cast<std::size_t>(width) * height;
    constexpr std::size_t clut_size = 1024U;
    const std::size_t picture_size = picture_header + image_size + clut_size;
    std::vector<std::uint8_t> bytes(file_header + picture_size, 0U);
    bytes[0] = 'T'; bytes[1] = 'I'; bytes[2] = 'M'; bytes[3] = '2';
    bytes[4] = 4U; bytes[5] = 0U;
    put_u16(bytes, 6U, 1U);
    put_u32(bytes, file_header, static_cast<std::uint32_t>(picture_size));
    put_u32(bytes, file_header + 4U, static_cast<std::uint32_t>(clut_size));
    put_u32(bytes, file_header + 8U, static_cast<std::uint32_t>(image_size));
    put_u16(bytes, file_header + 12U, static_cast<std::uint16_t>(picture_header));
    put_u16(bytes, file_header + 14U, 256U);
    bytes[file_header + 16U] = 0U;
    bytes[file_header + 17U] = 1U;
    bytes[file_header + 18U] = 3U;
    bytes[file_header + 19U] = 5U;
    put_u16(bytes, file_header + 20U, static_cast<std::uint16_t>(width));
    put_u16(bytes, file_header + 22U, static_cast<std::uint16_t>(height));
    put_u32(bytes, file_header + 24U, 19U << 20U); // validated PSMT8 profile
    const std::size_t image_offset = file_header + picture_header;
    for (std::size_t i = 0; i < image_size; ++i) bytes[image_offset + i] = static_cast<std::uint8_t>(i % 4U);
    const std::size_t palette_offset = image_offset + image_size;
    const std::uint8_t colors[4][4] = {
        {255U, 10U, 20U, 128U}, {10U, 255U, 20U, 128U},
        {10U, 20U, 255U, 128U}, {0U, 0U, 0U, 0U}
    };
    for (std::size_t i = 0; i < 4U; ++i) {
        for (std::size_t c = 0; c < 4U; ++c) bytes[palette_offset + i * 4U + c] = colors[i][c];
    }
    return bytes;
}
}

int main() {
    ps2asset::texture::RgbaImage source;
    source.width = 17U;
    source.height = 9U;
    source.pixels.resize(static_cast<std::size_t>(source.width) * source.height * 4U);
    for (std::size_t i = 0; i < source.pixels.size(); i += 4U) {
        source.pixels[i] = static_cast<std::uint8_t>((i * 7U) & 0xFFU);
        source.pixels[i + 1U] = static_cast<std::uint8_t>((i * 3U) & 0xFFU);
        source.pixels[i + 2U] = static_cast<std::uint8_t>((i * 11U) & 0xFFU);
        source.pixels[i + 3U] = static_cast<std::uint8_t>((i * 5U) & 0xFFU);
    }
    const auto png = ps2asset::image::encode_png(source);
    const auto decoded = ps2asset::image::decode_png(png);
    assert(decoded.valid());
    assert(decoded.width == source.width && decoded.height == source.height);
    assert(decoded.pixels == source.pixels);

    bool malformed_rejected = false;
    try { (void)ps2asset::image::decode_png({0U, 1U, 2U, 3U, 4U}); }
    catch (const std::exception&) { malformed_rejected = true; }
    assert(malformed_rejected);

    auto tim2 = ps2asset::texture::Tim2File::parse(fixture_tm2());
    const auto replacement = ps2asset::image::decode_png(ps2asset::image::encode_png(tim2.decode_picture(0U)));
    const auto original_size = tim2.bytes().size();
    tim2.replace_picture_rgba8_palette(0U, replacement);
    assert(tim2.bytes().size() == original_size);
    const auto reparsed = ps2asset::texture::Tim2File::parse(tim2.bytes());
    assert(reparsed.decode_picture(0U).pixels == replacement.pixels);

    // Exercise the median-cut path with more than 256 source colors.
    constexpr std::uint16_t width = 32U;
    constexpr std::uint16_t height = 32U;
    auto quantize_tim2 = ps2asset::texture::Tim2File::parse(fixture_tm2(width, height));
    ps2asset::texture::RgbaImage gradient;
    gradient.width = width;
    gradient.height = height;
    gradient.pixels.resize(static_cast<std::size_t>(width) * height * 4U);
    std::unordered_set<std::uint32_t> source_colors;
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const auto at = (static_cast<std::size_t>(y) * width + x) * 4U;
            gradient.pixels[at] = static_cast<std::uint8_t>((x * 7U + y * 3U) & 0xFFU);
            gradient.pixels[at + 1U] = static_cast<std::uint8_t>((x * 13U + y * 9U) & 0xFFU);
            gradient.pixels[at + 2U] = static_cast<std::uint8_t>((x * 17U + y * 19U) & 0xFFU);
            gradient.pixels[at + 3U] = 255U;
            const auto rgba = static_cast<std::uint32_t>(gradient.pixels[at]) |
                (static_cast<std::uint32_t>(gradient.pixels[at + 1U]) << 8U) |
                (static_cast<std::uint32_t>(gradient.pixels[at + 2U]) << 16U) | 0xFF000000U;
            source_colors.insert(rgba);
        }
    }
    assert(source_colors.size() > 256U);
    const auto imported = ps2asset::image::decode_png(ps2asset::image::encode_png(gradient));
    quantize_tim2.replace_picture_rgba8_palette(0U, imported);
    const auto quantized = ps2asset::texture::Tim2File::parse(quantize_tim2.bytes()).decode_picture(0U);
    assert(quantized.valid() && quantized.width == gradient.width && quantized.height == gradient.height);
    std::unordered_set<std::uint32_t> colors;
    for (std::size_t i = 0; i < quantized.pixels.size(); i += 4U) {
        const auto rgba = static_cast<std::uint32_t>(quantized.pixels[i]) |
            (static_cast<std::uint32_t>(quantized.pixels[i + 1U]) << 8U) |
            (static_cast<std::uint32_t>(quantized.pixels[i + 2U]) << 16U) |
            (static_cast<std::uint32_t>(quantized.pixels[i + 3U]) << 24U);
        colors.insert(rgba);
    }
    assert(colors.size() <= 256U);

    std::cout << "PNG encode/decode and TIM2 PNG replacement tests passed.\n";
    return 0;
}
