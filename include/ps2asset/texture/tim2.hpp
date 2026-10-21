#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ps2asset::texture {

struct RgbaImage {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    // Pixels are tightly packed RGBA8 in row-major order.
    std::vector<std::uint8_t> pixels;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] std::size_t pixel_count() const noexcept;
};

struct Tim2Picture {
    std::size_t file_offset = 0;
    std::size_t image_offset = 0;
    std::size_t clut_offset = 0;
    std::uint32_t total_size = 0;
    std::uint32_t clut_size = 0;
    std::uint32_t image_size = 0;
    std::uint16_t header_size = 0;
    std::uint16_t clut_colors = 0;
    std::uint8_t picture_format = 0;
    std::uint8_t mipmap_count = 0;
    std::uint8_t clut_type = 0;
    std::uint8_t image_type = 0;
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::uint64_t gs_tex0 = 0;

    [[nodiscard]] std::uint8_t psm() const noexcept;
    [[nodiscard]] std::string image_type_name() const;
    [[nodiscard]] bool supports_rgba8_palette_replace() const noexcept;
};

class Tim2File {
public:
    static Tim2File parse(std::vector<std::uint8_t> bytes);

    [[nodiscard]] const std::vector<std::uint8_t>& bytes() const noexcept { return bytes_; }
    [[nodiscard]] const std::vector<Tim2Picture>& pictures() const noexcept { return pictures_; }
    [[nodiscard]] std::uint8_t version() const noexcept { return version_; }
    [[nodiscard]] std::uint8_t format_type() const noexcept { return format_type_; }
    [[nodiscard]] std::uint16_t picture_count() const noexcept {
        return static_cast<std::uint16_t>(pictures_.size());
    }

    [[nodiscard]] RgbaImage decode_picture(std::size_t index) const;

    // Replaces the pixels of a supported 8-bit indexed picture and its RGBA32 CLUT.
    // All container/header fields and the file size are preserved. The input image
    // must have exactly the picture's dimensions; resizing is a UI-level decision.
    void replace_picture_rgba8_palette(std::size_t index, const RgbaImage& image);

private:
    std::vector<std::uint8_t> bytes_;
    std::vector<Tim2Picture> pictures_;
    std::uint8_t version_ = 0;
    std::uint8_t format_type_ = 0;
};

[[nodiscard]] std::vector<std::uint8_t> read_binary_file(const std::string& path,
                                                          std::uint64_t max_size = 1024ULL * 1024ULL * 1024ULL);
void write_binary_file_atomic(const std::string& path, const std::vector<std::uint8_t>& bytes);

}  // namespace ps2asset::texture
