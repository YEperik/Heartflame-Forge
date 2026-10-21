#pragma once

#include "ps2asset/texture/tim2.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace ps2asset::image {

// PNG conversion utilities backed by libpng's simplified API. Images are RGBA8,
// row-major, and capped by the same 64-megapixel limit as the texture core.
[[nodiscard]] std::vector<std::uint8_t> encode_png(const texture::RgbaImage& image);
[[nodiscard]] texture::RgbaImage decode_png(const std::vector<std::uint8_t>& bytes);
[[nodiscard]] texture::RgbaImage read_png(const std::string& path);
void write_png(const std::string& path, const texture::RgbaImage& image);

}  // namespace ps2asset::image
