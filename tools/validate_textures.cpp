#include "ps2asset/archive/zip_reader.hpp"
#include "ps2asset/texture/tim2.hpp"
#ifdef PS2ASSET_HAS_PNG
#include "ps2asset/image/png.hpp"
#endif

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using ps2asset::texture::RgbaImage;
using ps2asset::texture::Tim2File;
using ps2asset::texture::Tim2Picture;

struct AssetInput {
    std::string name;
    std::vector<std::uint8_t> bytes;
};

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

RgbaImage make_pattern(std::uint32_t width, std::uint32_t height, std::size_t picture_index) {
    RgbaImage image;
    image.width = width;
    image.height = height;
    const auto pixels = static_cast<std::uint64_t>(width) * height;
    if (pixels == 0U || pixels > 64ULL * 1024ULL * 1024ULL) {
        throw std::runtime_error("Unsafe texture dimensions while making validation pattern");
    }
    image.pixels.resize(static_cast<std::size_t>(pixels) * 4U);
    constexpr std::uint8_t colors[4][4] = {
        {255U, 0U, 0U, 255U},
        {0U, 255U, 0U, 255U},
        {0U, 0U, 255U, 255U},
        {0U, 0U, 0U, 0U}
    };
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::size_t index = (static_cast<std::size_t>(y) * width + x) * 4U;
            const auto color = static_cast<std::size_t>(((x / 8U) + (y / 8U) + picture_index) % 4U);
            std::copy(std::begin(colors[color]), std::end(colors[color]), image.pixels.begin() + static_cast<std::ptrdiff_t>(index));
        }
    }
    return image;
}

bool is_mutable_payload_byte(std::size_t offset, const Tim2Picture& picture) {
    const auto pixel_count = static_cast<std::uint64_t>(picture.width) * picture.height;
    const auto image_begin = picture.image_offset;
    const auto image_end = image_begin + static_cast<std::size_t>(pixel_count);
    const auto clut_begin = picture.clut_offset;
    const auto clut_end = clut_begin + 1024U;
    return (offset >= image_begin && offset < image_end) ||
           (offset >= clut_begin && offset < clut_end);
}

void verify_only_target_payload_changed(const std::vector<std::uint8_t>& before,
                                        const std::vector<std::uint8_t>& after,
                                        const Tim2Picture& picture) {
    if (before.size() != after.size()) {
        throw std::runtime_error("Replacement changed total TIM2 file size");
    }
    for (std::size_t i = 0; i < before.size(); ++i) {
        if (!is_mutable_payload_byte(i, picture) && before[i] != after[i]) {
            throw std::runtime_error("Replacement unexpectedly changed byte outside the target image/CLUT payload at offset " + std::to_string(i));
        }
    }
}

void validate_asset(const AssetInput& asset, std::size_t& picture_count,
                    std::size_t& supported_count, std::size_t& replaced_count,
                    std::size_t& unsupported_count, bool png_roundtrip = false) {
    auto file = Tim2File::parse(asset.bytes);
    const auto original_size = file.bytes().size();
    const auto initial_pictures = file.pictures();
    const auto unsupported_before = unsupported_count;
    std::vector<std::optional<RgbaImage>> expected(initial_pictures.size());

    for (std::size_t i = 0; i < initial_pictures.size(); ++i) {
        const auto& picture = initial_pictures[i];
        ++picture_count;
        if (!picture.supports_rgba8_palette_replace()) {
            ++unsupported_count;
            std::cout << "  SKIP " << asset.name << " picture " << i << ": profile not supported by current replace module\n";
            continue;
        }

        const auto decoded_original = file.decode_picture(i);
        if (!decoded_original.valid() || decoded_original.width != picture.width || decoded_original.height != picture.height) {
            throw std::runtime_error("Decoded image dimensions/buffer are invalid in " + asset.name + " picture " + std::to_string(i));
        }
        ++supported_count;
        RgbaImage replacement;
        if (png_roundtrip) {
#ifdef PS2ASSET_HAS_PNG
            const auto png_bytes = ps2asset::image::encode_png(decoded_original);
            replacement = ps2asset::image::decode_png(png_bytes);
            if (replacement.width != decoded_original.width || replacement.height != decoded_original.height ||
                replacement.pixels != decoded_original.pixels) {
                throw std::runtime_error("Lossless PNG encode/decode mismatch in " + asset.name + " picture " + std::to_string(i));
            }
#else
            throw std::runtime_error("PNG round-trip mode requires a build with libpng development files");
#endif
        } else {
            replacement = make_pattern(picture.width, picture.height, i);
        }
        const auto before = file.bytes();
        file.replace_picture_rgba8_palette(i, replacement);
        verify_only_target_payload_changed(before, file.bytes(), picture);

        const auto roundtrip = file.decode_picture(i);
        if (roundtrip.width != replacement.width || roundtrip.height != replacement.height ||
            roundtrip.pixels != replacement.pixels) {
            throw std::runtime_error("In-memory replacement decode mismatch in " + asset.name + " picture " + std::to_string(i));
        }
        expected[i] = replacement;
        ++replaced_count;
    }

    if (file.bytes().size() != original_size) {
        throw std::runtime_error("TIM2 file size changed during replacement in " + asset.name);
    }
    auto reparsed = Tim2File::parse(file.bytes());
    if (reparsed.picture_count() != initial_pictures.size()) {
        throw std::runtime_error("Picture count changed after reparsing " + asset.name);
    }
    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (!expected[i]) continue;
        const auto image = reparsed.decode_picture(i);
        if (image.pixels != expected[i]->pixels || image.width != expected[i]->width || image.height != expected[i]->height) {
            throw std::runtime_error("Save/reparse/decode round-trip mismatch in " + asset.name + " picture " + std::to_string(i));
        }
    }
    const bool skipped_unsupported = unsupported_count != unsupported_before;
    std::cout << "  " << (skipped_unsupported ? "PARTIAL " : "PASS ") << asset.name
              << " (" << reparsed.picture_count() << " picture(s), " << original_size << " bytes; ";
    if (skipped_unsupported) {
        std::cout << (png_roundtrip ? "PNG encode/decode checked where supported" : "replacement checked where supported")
                  << "; unsupported picture profile(s) were skipped)\n";
    } else {
        std::cout << (png_roundtrip ? "PNG export/import + replacement + reparse" : "synthetic replacement + reparse")
                  << " passed)\n";
    }
}


void put_u16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
}

void put_u32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned int i = 0; i < 4U; ++i) {
        bytes[offset + i] = static_cast<std::uint8_t>((value >> (8U * i)) & 0xFFU);
    }
}

std::vector<std::uint8_t> make_self_test_tm2() {
    constexpr std::size_t file_header = 16U;
    constexpr std::size_t picture_header = 48U;
    constexpr std::size_t width = 16U;
    constexpr std::size_t height = 16U;
    constexpr std::size_t image_size = width * height;
    constexpr std::size_t clut_size = 1024U;
    constexpr std::size_t picture_size = picture_header + image_size + clut_size;
    std::vector<std::uint8_t> bytes(file_header + picture_size, 0U);
    bytes[0] = 'T'; bytes[1] = 'I'; bytes[2] = 'M'; bytes[3] = '2';
    bytes[4] = 4U; bytes[5] = 0U;
    put_u16(bytes, 6U, 1U);
    put_u32(bytes, file_header, static_cast<std::uint32_t>(picture_size));
    put_u32(bytes, file_header + 4U, static_cast<std::uint32_t>(clut_size));
    put_u32(bytes, file_header + 8U, static_cast<std::uint32_t>(image_size));
    put_u16(bytes, file_header + 12U, static_cast<std::uint16_t>(picture_header));
    put_u16(bytes, file_header + 14U, 256U);
    bytes[file_header + 16U] = 0U; // picture format
    bytes[file_header + 17U] = 1U; // one mip level, as used by this profile
    bytes[file_header + 18U] = 3U; // RGBA32 CLUT
    bytes[file_header + 19U] = 5U; // indexed 8-bit
    put_u16(bytes, file_header + 20U, static_cast<std::uint16_t>(width));
    put_u16(bytes, file_header + 22U, static_cast<std::uint16_t>(height));
    // Mark this synthetic fixture as the PSMT8 profile used by the decoder.
    put_u32(bytes, file_header + 24U, 19U << 20U);
    const auto image_offset = file_header + picture_header;
    for (std::size_t i = 0; i < image_size; ++i) bytes[image_offset + i] = static_cast<std::uint8_t>(i % 4U);
    const auto clut_offset = image_offset + image_size;
    const std::uint8_t palette[4][4] = {
        {255U, 0U, 0U, 128U}, {0U, 255U, 0U, 128U},
        {0U, 0U, 255U, 128U}, {0U, 0U, 0U, 0U}
    };
    for (std::size_t color = 0; color < 4U; ++color) {
        std::copy(std::begin(palette[color]), std::end(palette[color]),
                  bytes.begin() + static_cast<std::ptrdiff_t>(clut_offset + color * 4U));
    }
    return bytes;
}

void usage() {
    std::cerr << "Heartflame Forge - read-only TIM2 replacement round-trip validator\n"
              << "Usage: ps2asset-texture-validate <input.tm2|archive.zip>\n"
              << "       ps2asset-texture-validate --self-test\n"
              << "       ps2asset-texture-validate --png-roundtrip <input.tm2|archive.zip>\n"
              << "Tests run in memory only; no input or output files are written. PNG mode requires libpng.\n";
}
}  // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--self-test") {
        try {
            std::size_t picture_count = 0, supported_count = 0, replaced_count = 0, unsupported_count = 0;
            const AssetInput fixture{"<generated TIM2 fixture>", make_self_test_tm2()};
            validate_asset(fixture, picture_count, supported_count, replaced_count, unsupported_count);
            if (picture_count != 1U || supported_count != 1U || replaced_count != 1U || unsupported_count != 0U) {
                throw std::runtime_error("Self-test did not exercise exactly one supported replacement round-trip");
            }
            std::cout << "Validator self-test passed; no files written.\n";
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "self-test failed: " << error.what() << '\n';
            return 1;
        }
    }

    bool png_roundtrip = false;
    std::string input_path;
    if (argc == 2) {
        input_path = argv[1];
    } else if (argc == 3 && std::string(argv[1]) == "--png-roundtrip") {
#ifdef PS2ASSET_HAS_PNG
        png_roundtrip = true;
        input_path = argv[2];
#else
        std::cerr << "PNG round-trip mode requires a build with libpng development files.\n";
        return 2;
#endif
    } else {
        usage();
        return 2;
    }
    const auto extension = lower(input_path.substr(input_path.find_last_of('.') == std::string::npos
        ? input_path.size() : input_path.find_last_of('.')));
    std::vector<AssetInput> assets;
    try {
        if (extension == ".zip") {
            const auto textures = ps2asset::archive::read_tim2_entries_from_zip(input_path);
            assets.reserve(textures.size());
            for (const auto& texture : textures) assets.push_back({texture.entry_name, texture.bytes});
        } else if (extension == ".tm2") {
            assets.push_back({input_path, ps2asset::texture::read_binary_file(input_path, 32ULL * 1024ULL * 1024ULL)});
        } else {
            usage();
            return 2;
        }

        if (assets.empty()) throw std::runtime_error("No TIM2 inputs found");
        std::size_t picture_count = 0, supported_count = 0, replaced_count = 0, unsupported_count = 0;
        std::size_t failed_count = 0;
        for (const auto& asset : assets) {
            try {
                validate_asset(asset, picture_count, supported_count, replaced_count, unsupported_count, png_roundtrip);
            } catch (const std::exception& error) {
                ++failed_count;
                std::cerr << "  FAIL " << asset.name << ": " << error.what() << '\n';
            }
        }
        std::cout << "\nSummary (" << (png_roundtrip ? "PNG round-trip" : "synthetic replacement") << ")\n"
                  << "  Assets: " << assets.size() << '\n'
                  << "  Pictures parsed: " << picture_count << '\n'
                  << "  Pictures in supported profile: " << supported_count << '\n'
                  << "  Replacement + reparse round-trips: " << replaced_count << '\n'
                  << "  Unsupported profile pictures: " << unsupported_count << '\n'
                  << "  Failed assets: " << failed_count << '\n'
                  << "  Input files modified: 0\n";
        if (failed_count != 0U) return 1;
        if (supported_count == 0U) {
            std::cout.flush();
            std::cerr << "No pictures matched the currently supported replacement profile.\n";
            return 3;
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
