#include "ps2asset/image/png.hpp"

#include "ps2asset/texture/tim2.hpp"

#include <png.h>

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace ps2asset::image {
namespace {
constexpr std::uint64_t kMaxPngBytes = 512ULL * 1024ULL * 1024ULL;

void validate_image_dimensions(png_uint_32 width, png_uint_32 height) {
    if (width == 0U || height == 0U) {
        throw std::runtime_error("PNG image has zero width or height");
    }
    const auto pixels = static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
    if (pixels > 64ULL * 1024ULL * 1024ULL || pixels > std::numeric_limits<std::size_t>::max() / 4U) {
        throw std::runtime_error("PNG dimensions exceed the 64-megapixel safety limit");
    }
}

std::string png_error(const png_image& image, const char* operation) {
    return std::string(operation) + ": " + (image.message[0] != '\0' ? image.message : "libpng returned an unspecified error");
}
}  // namespace

std::vector<std::uint8_t> encode_png(const texture::RgbaImage& source) {
    if (!source.valid()) throw std::runtime_error("Cannot encode invalid or oversized RGBA image as PNG");
    if (source.width > std::numeric_limits<png_uint_32>::max() ||
        source.height > std::numeric_limits<png_uint_32>::max()) {
        throw std::runtime_error("RGBA dimensions cannot be represented by PNG");
    }

    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    image.width = static_cast<png_uint_32>(source.width);
    image.height = static_cast<png_uint_32>(source.height);
    image.format = PNG_FORMAT_RGBA;

    png_alloc_size_t size = 0;
    if (png_image_write_to_memory(&image, nullptr, &size, 0, source.pixels.data(), 0, nullptr) == 0) {
        const auto message = png_error(image, "Cannot calculate PNG output size");
        png_image_free(&image);
        throw std::runtime_error(message);
    }
    if (size == 0U || static_cast<std::uint64_t>(size) > kMaxPngBytes) {
        png_image_free(&image);
        throw std::runtime_error("PNG output exceeds the configured 512 MiB limit");
    }

    std::vector<std::uint8_t> output(static_cast<std::size_t>(size));
    if (png_image_write_to_memory(&image, output.data(), &size, 0, source.pixels.data(), 0, nullptr) == 0) {
        const auto message = png_error(image, "Cannot encode PNG image");
        png_image_free(&image);
        throw std::runtime_error(message);
    }
    output.resize(static_cast<std::size_t>(size));
    png_image_free(&image);
    return output;
}

texture::RgbaImage decode_png(const std::vector<std::uint8_t>& bytes) {
    if (bytes.empty()) throw std::runtime_error("PNG input is empty");
    if (static_cast<std::uint64_t>(bytes.size()) > kMaxPngBytes) {
        throw std::runtime_error("PNG input exceeds the configured 512 MiB limit");
    }

    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    if (png_image_begin_read_from_memory(&image, bytes.data(), static_cast<png_alloc_size_t>(bytes.size())) == 0) {
        const auto message = png_error(image, "Cannot read PNG header");
        png_image_free(&image);
        throw std::runtime_error(message);
    }
    try {
        validate_image_dimensions(image.width, image.height);
        image.format = PNG_FORMAT_RGBA;
        const auto output_bytes = static_cast<std::uint64_t>(image.width) * image.height * 4ULL;
        if (output_bytes > std::numeric_limits<std::size_t>::max()) {
            throw std::runtime_error("PNG decoded image is too large for this platform");
        }
        texture::RgbaImage result;
        result.width = image.width;
        result.height = image.height;
        result.pixels.resize(static_cast<std::size_t>(output_bytes));
        if (png_image_finish_read(&image, nullptr, result.pixels.data(), 0, nullptr) == 0) {
            throw std::runtime_error(png_error(image, "Cannot decode PNG pixel data"));
        }
        png_image_free(&image);
        if (!result.valid()) throw std::runtime_error("Decoded PNG has an invalid RGBA buffer");
        return result;
    } catch (...) {
        png_image_free(&image);
        throw;
    }
}

texture::RgbaImage read_png(const std::string& path) {
    return decode_png(texture::read_binary_file(path, kMaxPngBytes));
}

void write_png(const std::string& path, const texture::RgbaImage& image) {
    texture::write_binary_file_atomic(path, encode_png(image));
}

}  // namespace ps2asset::image
