#include "ps2asset/texture/tim2.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <numeric>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace ps2asset::texture {
namespace {
constexpr std::size_t kTim2FileHeaderSize = 16;
constexpr std::size_t kTim2PictureHeaderSize = 48;
constexpr std::uint64_t kMaxPixels = 64ULL * 1024ULL * 1024ULL;

void require_range(std::size_t total, std::size_t offset, std::size_t length, const char* what) {
    if (offset > total || length > total - offset) {
        throw std::runtime_error(std::string("TIM2: ") + what + " is outside the file bounds");
    }
}

std::uint16_t read_u16_le(const std::vector<std::uint8_t>& b, std::size_t p) {
    require_range(b.size(), p, 2, "16-bit field");
    return static_cast<std::uint16_t>(b[p]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(b[p + 1]) << 8U);
}

std::uint32_t read_u32_le(const std::vector<std::uint8_t>& b, std::size_t p) {
    require_range(b.size(), p, 4, "32-bit field");
    return static_cast<std::uint32_t>(b[p]) |
           (static_cast<std::uint32_t>(b[p + 1]) << 8U) |
           (static_cast<std::uint32_t>(b[p + 2]) << 16U) |
           (static_cast<std::uint32_t>(b[p + 3]) << 24U);
}

std::uint64_t read_u64_le(const std::vector<std::uint8_t>& b, std::size_t p) {
    const auto lo = static_cast<std::uint64_t>(read_u32_le(b, p));
    const auto hi = static_cast<std::uint64_t>(read_u32_le(b, p + 4));
    return lo | (hi << 32U);
}


std::uint8_t ps2_alpha_to_rgba(std::uint8_t a) {
    return static_cast<std::uint8_t>(std::min<unsigned int>(255U, static_cast<unsigned int>(a) * 2U));
}

std::uint8_t rgba_alpha_to_ps2(std::uint8_t a) {
    return static_cast<std::uint8_t>((static_cast<unsigned int>(a) * 128U + 127U) / 255U);
}

struct Color {
    std::uint8_t r = 0, g = 0, b = 0, a = 255;
    std::uint32_t count = 0;
    [[nodiscard]] std::uint32_t key() const noexcept {
        return static_cast<std::uint32_t>(r) |
               (static_cast<std::uint32_t>(g) << 8U) |
               (static_cast<std::uint32_t>(b) << 16U) |
               (static_cast<std::uint32_t>(a) << 24U);
    }
};

unsigned int channel_value(const Color& c, int channel) {
    switch (channel) {
        case 0: return c.r;
        case 1: return c.g;
        case 2: return c.b;
        default: return c.a;
    }
}

struct ColorBox {
    std::vector<std::size_t> items;
    std::array<unsigned int, 4> range{};
    std::uint64_t population = 0;
    unsigned int score = 0;
};

void update_box(ColorBox& box, const std::vector<Color>& colors) {
    box.range = {0U, 0U, 0U, 0U};
    box.population = 0;
    std::array<unsigned int, 4> lo{255U, 255U, 255U, 255U};
    std::array<unsigned int, 4> hi{0U, 0U, 0U, 0U};
    for (const auto i : box.items) {
        const auto& c = colors[i];
        box.population += c.count;
        for (int ch = 0; ch < 4; ++ch) {
            const auto value = channel_value(c, ch);
            lo[static_cast<std::size_t>(ch)] = std::min(lo[static_cast<std::size_t>(ch)], value);
            hi[static_cast<std::size_t>(ch)] = std::max(hi[static_cast<std::size_t>(ch)], value);
        }
    }
    for (std::size_t ch = 0; ch < 4; ++ch) box.range[ch] = hi[ch] - lo[ch];
    box.score = *std::max_element(box.range.begin(), box.range.end());
}

int widest_channel(const ColorBox& box) {
    return static_cast<int>(std::distance(box.range.begin(), std::max_element(box.range.begin(), box.range.end())));
}

std::vector<Color> make_palette(const RgbaImage& image) {
    std::map<std::uint32_t, std::uint32_t> histogram;
    for (std::size_t i = 0; i < image.pixels.size(); i += 4U) {
        const auto r = image.pixels[i];
        const auto g = image.pixels[i + 1U];
        const auto b = image.pixels[i + 2U];
        const auto a = image.pixels[i + 3U];
        const std::uint32_t key = static_cast<std::uint32_t>(r) |
            (static_cast<std::uint32_t>(g) << 8U) |
            (static_cast<std::uint32_t>(b) << 16U) |
            (static_cast<std::uint32_t>(a) << 24U);
        ++histogram[key];
    }

    std::vector<Color> colors;
    colors.reserve(histogram.size());
    for (const auto& entry : histogram) {
        const auto key = entry.first;
        colors.push_back(Color{
            static_cast<std::uint8_t>(key & 0xFFU),
            static_cast<std::uint8_t>((key >> 8U) & 0xFFU),
            static_cast<std::uint8_t>((key >> 16U) & 0xFFU),
            static_cast<std::uint8_t>((key >> 24U) & 0xFFU),
            entry.second});
    }
    if (colors.empty()) throw std::runtime_error("Cannot import an empty image");

    std::vector<Color> palette;
    if (colors.size() <= 256U) {
        palette = colors;
        for (auto& color : palette) color.count = 1U;
    } else {
        ColorBox initial;
        initial.items.resize(colors.size());
        std::iota(initial.items.begin(), initial.items.end(), 0U);
        update_box(initial, colors);
        std::vector<ColorBox> boxes;
        boxes.push_back(std::move(initial));

        while (boxes.size() < 256U) {
            auto candidate = boxes.end();
            for (auto it = boxes.begin(); it != boxes.end(); ++it) {
                if (it->items.size() < 2U) continue;
                if (candidate == boxes.end() || it->score > candidate->score ||
                    (it->score == candidate->score && it->population > candidate->population)) {
                    candidate = it;
                }
            }
            if (candidate == boxes.end() || candidate->score == 0U) break;

            ColorBox left;
            ColorBox right;
            const int channel = widest_channel(*candidate);
            auto items = std::move(candidate->items);
            std::stable_sort(items.begin(), items.end(), [&](std::size_t a, std::size_t b) {
                const auto av = channel_value(colors[a], channel);
                const auto bv = channel_value(colors[b], channel);
                return av == bv ? colors[a].key() < colors[b].key() : av < bv;
            });
            const std::uint64_t half = (candidate->population + 1U) / 2U;
            std::uint64_t accumulated = 0;
            std::size_t split = 0;
            for (; split + 1U < items.size(); ++split) {
                accumulated += colors[items[split]].count;
                if (accumulated >= half) { ++split; break; }
            }
            split = std::clamp(split, static_cast<std::size_t>(1U), items.size() - 1U);
            left.items.assign(items.begin(), items.begin() + static_cast<std::ptrdiff_t>(split));
            right.items.assign(items.begin() + static_cast<std::ptrdiff_t>(split), items.end());
            update_box(left, colors);
            update_box(right, colors);
            *candidate = std::move(left);
            boxes.push_back(std::move(right));
        }

        palette.reserve(256U);
        for (const auto& box : boxes) {
            std::array<std::uint64_t, 4> sums{};
            std::uint64_t total = 0;
            for (const auto index : box.items) {
                const auto& c = colors[index];
                total += c.count;
                sums[0] += static_cast<std::uint64_t>(c.r) * c.count;
                sums[1] += static_cast<std::uint64_t>(c.g) * c.count;
                sums[2] += static_cast<std::uint64_t>(c.b) * c.count;
                sums[3] += static_cast<std::uint64_t>(c.a) * c.count;
            }
            if (total == 0U) total = 1U;
            palette.push_back(Color{
                static_cast<std::uint8_t>((sums[0] + total / 2U) / total),
                static_cast<std::uint8_t>((sums[1] + total / 2U) / total),
                static_cast<std::uint8_t>((sums[2] + total / 2U) / total),
                static_cast<std::uint8_t>((sums[3] + total / 2U) / total), 1U});
        }
    }

    while (palette.size() < 256U) {
        palette.push_back(palette.back());
    }
    return palette;
}

std::size_t nearest_palette_color(const std::uint8_t* rgba, const std::vector<Color>& palette) {
    std::uint64_t best_distance = std::numeric_limits<std::uint64_t>::max();
    std::size_t best_index = 0;
    for (std::size_t i = 0; i < palette.size(); ++i) {
        const auto& c = palette[i];
        const int dr = static_cast<int>(rgba[0]) - static_cast<int>(c.r);
        const int dg = static_cast<int>(rgba[1]) - static_cast<int>(c.g);
        const int db = static_cast<int>(rgba[2]) - static_cast<int>(c.b);
        const int da = static_cast<int>(rgba[3]) - static_cast<int>(c.a);
        const auto distance = static_cast<std::uint64_t>(dr * dr + dg * dg + db * db + 2 * da * da);
        if (distance < best_distance) {
            best_distance = distance;
            best_index = i;
            if (distance == 0U) break;
        }
    }
    return best_index;
}
}  // namespace

bool RgbaImage::valid() const noexcept {
    if (width == 0U || height == 0U) return false;
    const auto pixels_count = static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
    return pixels_count <= kMaxPixels && pixels_count <= std::numeric_limits<std::size_t>::max() / 4U &&
           pixels.size() == static_cast<std::size_t>(pixels_count * 4U);
}

std::size_t RgbaImage::pixel_count() const noexcept {
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
}

std::uint8_t Tim2Picture::psm() const noexcept {
    return static_cast<std::uint8_t>((gs_tex0 >> 20U) & 0x3FU);
}

std::string Tim2Picture::image_type_name() const {
    switch (image_type) {
        case 1: return "16-bit RGBA (A1B5G5R5)";
        case 2: return "24-bit RGB";
        case 3: return "32-bit RGBA";
        case 4: return "4-bit indexed";
        case 5: return "8-bit indexed";
        default: return "Unknown image type (" + std::to_string(image_type) + ")";
    }
}

bool Tim2Picture::supports_rgba8_palette_replace() const noexcept {
    const auto pixel_count = static_cast<std::uint64_t>(width) * height;
    return image_type == 5U && clut_type == 3U && clut_colors == 256U && clut_size == 1024U &&
           mipmap_count == 1U && header_size == kTim2PictureHeaderSize &&
           pixel_count > 0U && pixel_count <= image_size &&
           image_offset <= std::numeric_limits<std::size_t>::max() &&
           clut_offset <= std::numeric_limits<std::size_t>::max();
}

Tim2File Tim2File::parse(std::vector<std::uint8_t> bytes) {
    if (bytes.size() < kTim2FileHeaderSize) throw std::runtime_error("TIM2 file is shorter than the 16-byte file header");
    if (!(bytes[0] == 'T' && bytes[1] == 'I' && bytes[2] == 'M' && bytes[3] == '2')) {
        throw std::runtime_error("Invalid TIM2 signature (expected 'TIM2')");
    }

    Tim2File result;
    result.bytes_ = std::move(bytes);
    result.version_ = result.bytes_[4];
    result.format_type_ = result.bytes_[5];
    const auto count = read_u16_le(result.bytes_, 6);
    if (count == 0U) throw std::runtime_error("TIM2 picture count is zero");
    std::size_t picture_offset = result.format_type_ == 1U ? 0x80U : kTim2FileHeaderSize;
    require_range(result.bytes_.size(), picture_offset, kTim2PictureHeaderSize, "first picture header");
    result.pictures_.reserve(count);

    for (std::uint16_t i = 0; i < count; ++i) {
        require_range(result.bytes_.size(), picture_offset, kTim2PictureHeaderSize, "picture header");
        Tim2Picture picture;
        picture.file_offset = picture_offset;
        picture.total_size = read_u32_le(result.bytes_, picture_offset);
        picture.clut_size = read_u32_le(result.bytes_, picture_offset + 4U);
        picture.image_size = read_u32_le(result.bytes_, picture_offset + 8U);
        picture.header_size = read_u16_le(result.bytes_, picture_offset + 12U);
        picture.clut_colors = read_u16_le(result.bytes_, picture_offset + 14U);
        picture.picture_format = result.bytes_[picture_offset + 16U];
        picture.mipmap_count = result.bytes_[picture_offset + 17U];
        picture.clut_type = result.bytes_[picture_offset + 18U];
        picture.image_type = result.bytes_[picture_offset + 19U];
        picture.width = read_u16_le(result.bytes_, picture_offset + 20U);
        picture.height = read_u16_le(result.bytes_, picture_offset + 22U);
        picture.gs_tex0 = read_u64_le(result.bytes_, picture_offset + 24U);

        if (picture.total_size < kTim2PictureHeaderSize) throw std::runtime_error("TIM2 picture total_size is smaller than its fixed header");
        if (picture.header_size < kTim2PictureHeaderSize || picture.header_size > picture.total_size) {
            throw std::runtime_error("TIM2 picture header_size is invalid");
        }
        require_range(result.bytes_.size(), picture_offset, picture.total_size, "picture payload");
        if (picture.width == 0U || picture.height == 0U) throw std::runtime_error("TIM2 picture has zero width or height");
        const auto pixel_count = static_cast<std::uint64_t>(picture.width) * picture.height;
        if (pixel_count > kMaxPixels) throw std::runtime_error("TIM2 picture dimensions exceed the 64-megapixel safety limit");
        if (picture.image_size > picture.total_size || picture.clut_size > picture.total_size) {
            throw std::runtime_error("TIM2 image/CLUT size exceeds picture total_size");
        }
        if (static_cast<std::uint64_t>(picture.header_size) + picture.image_size + picture.clut_size > picture.total_size) {
            throw std::runtime_error("TIM2 header + image + CLUT sizes exceed picture total_size");
        }
        picture.image_offset = picture_offset + picture.header_size;
        picture.clut_offset = picture.image_offset + picture.image_size;
        require_range(result.bytes_.size(), picture.image_offset, picture.image_size, "image data");
        require_range(result.bytes_.size(), picture.clut_offset, picture.clut_size, "CLUT data");
        result.pictures_.push_back(picture);
        picture_offset += picture.total_size;
    }
    return result;
}

RgbaImage Tim2File::decode_picture(std::size_t index) const {
    if (index >= pictures_.size()) throw std::out_of_range("TIM2 picture index is out of range");
    const auto& p = pictures_[index];
    if (p.image_type != 5U || p.clut_type != 3U || p.clut_size < 1024U || p.clut_colors < 256U) {
        throw std::runtime_error("This first texture module decodes 8-bit indexed TIM2 pictures with an RGBA32 palette; this picture uses image_type=" +
                                 std::to_string(p.image_type) + ", clut_type=" + std::to_string(p.clut_type));
    }
    const auto count64 = static_cast<std::uint64_t>(p.width) * p.height;
    if (count64 > p.image_size) throw std::runtime_error("TIM2 image payload is shorter than width * height for 8-bit indexed pixels");
    RgbaImage image;
    image.width = p.width;
    image.height = p.height;
    image.pixels.resize(static_cast<std::size_t>(count64) * 4U);
    const auto& b = bytes_;
    for (std::size_t i = 0; i < static_cast<std::size_t>(count64); ++i) {
        const auto palette_index = static_cast<std::size_t>(b[p.image_offset + i]);
        const auto palette_offset = p.clut_offset + palette_index * 4U;
        image.pixels[i * 4U] = b[palette_offset];
        image.pixels[i * 4U + 1U] = b[palette_offset + 1U];
        image.pixels[i * 4U + 2U] = b[palette_offset + 2U];
        image.pixels[i * 4U + 3U] = ps2_alpha_to_rgba(b[palette_offset + 3U]);
    }
    return image;
}

void Tim2File::replace_picture_rgba8_palette(std::size_t index, const RgbaImage& image) {
    if (index >= pictures_.size()) throw std::out_of_range("TIM2 picture index is out of range");
    if (!image.valid()) throw std::runtime_error("Imported image buffer is invalid or too large");
    auto& p = pictures_[index];
    if (!p.supports_rgba8_palette_replace()) {
        throw std::runtime_error("This picture is not in the supported import profile (8-bit indexed, 256-color RGBA32 CLUT, one mip level, 48-byte header)");
    }
    if (image.width != p.width || image.height != p.height) {
        throw std::runtime_error("Imported image dimensions must match the target texture (" + std::to_string(p.width) + "x" +
                                 std::to_string(p.height) + "); resize the image in the GUI and try again");
    }

    auto palette = make_palette(image);
    std::vector<std::uint8_t> encoded_indices(image.pixel_count());
    for (std::size_t i = 0; i < image.pixel_count(); ++i) {
        encoded_indices[i] = static_cast<std::uint8_t>(nearest_palette_color(image.pixels.data() + i * 4U, palette));
    }

    require_range(bytes_.size(), p.image_offset, encoded_indices.size(), "replacement image data");
    require_range(bytes_.size(), p.clut_offset, 1024U, "replacement CLUT data");
    std::copy(encoded_indices.begin(), encoded_indices.end(), bytes_.begin() + static_cast<std::ptrdiff_t>(p.image_offset));
    for (std::size_t i = 0; i < 256U; ++i) {
        const auto offset = p.clut_offset + i * 4U;
        bytes_[offset] = palette[i].r;
        bytes_[offset + 1U] = palette[i].g;
        bytes_[offset + 2U] = palette[i].b;
        bytes_[offset + 3U] = rgba_alpha_to_ps2(palette[i].a);
    }
}

std::vector<std::uint8_t> read_binary_file(const std::string& path, std::uint64_t max_size) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec) throw std::runtime_error("Cannot get file size for '" + path + "': " + ec.message());
    if (size > max_size) throw std::runtime_error("File exceeds configured maximum size: " + path);
    if (size > std::numeric_limits<std::size_t>::max()) throw std::runtime_error("File is too large for this platform: " + path);
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open file: " + path);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    if (!bytes.empty()) input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!input && !bytes.empty()) throw std::runtime_error("Could not read the complete file: " + path);
    return bytes;
}

void write_binary_file_atomic(const std::string& path, const std::vector<std::uint8_t>& bytes) {
    const std::filesystem::path destination(path);
    const auto parent = destination.has_parent_path() ? destination.parent_path() : std::filesystem::current_path();
    const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto temp = parent / (destination.filename().string() + ".tmp." + std::to_string(tick));
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("Cannot create temporary output file next to: " + path);
        if (!bytes.empty()) out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        out.flush();
        if (!out) {
            out.close();
            std::error_code ignored;
            std::filesystem::remove(temp, ignored);
            throw std::runtime_error("Failed while writing temporary output file: " + path);
        }
    }
    std::error_code ec;
    std::filesystem::rename(temp, destination, ec);
    if (ec) {
        // On Windows rename may not replace an existing file. Do not delete the
        // destination: preserve user data and report the failure explicitly.
        std::error_code ignored;
        std::filesystem::remove(temp, ignored);
        throw std::runtime_error("Could not atomically install output file '" + path + "': " + ec.message());
    }
}

}  // namespace ps2asset::texture
