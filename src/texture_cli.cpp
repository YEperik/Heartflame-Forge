#include "ps2asset/texture/tim2.hpp"
#ifdef PS2ASSET_HAS_PNG
#include "ps2asset/image/png.hpp"
#endif

#include <algorithm>
#include <cstdint>
#include <exception>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>

namespace {
void usage() {
    std::cerr << "PS2 Asset Workbench - TIM2 CLI helper\n"
              << "Usage:\n"
              << "  ps2asset-texture info <input.tm2>\n"
              << "  ps2asset-texture export <input.tm2> <picture-index> <output.ppm>\n"
#ifdef PS2ASSET_HAS_PNG
              << "  ps2asset-texture export-png <input.tm2> <picture-index> <output.png>\n"
              << "  ps2asset-texture replace-png <input.tm2> <picture-index> <input.png> <output.tm2>\n";
#else
              << "  PNG commands are disabled (build with libpng development files).\n";
#endif
}

void write_ppm(const std::string& path, const ps2asset::texture::RgbaImage& image) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("Cannot create output image: " + path);
    out << "P6\n" << image.width << ' ' << image.height << "\n255\n";
    for (std::size_t i = 0; i < image.pixels.size(); i += 4U) {
        // PPM has no alpha. Composite over a neutral checker pattern for this CLI preview.
        const std::size_t pixel = i / 4U;
        const std::size_t x = pixel % image.width;
        const std::size_t y = pixel / image.width;
        const std::uint8_t bg = (((x / 8U) + (y / 8U)) % 2U == 0U) ? 48U : 72U;
        const unsigned int a = image.pixels[i + 3U];
        for (std::size_t c = 0; c < 3U; ++c) {
            const unsigned int v = image.pixels[i + c];
            const unsigned int mixed = (v * a + static_cast<unsigned int>(bg) * (255U - a) + 127U) / 255U;
            out.put(static_cast<char>(mixed));
        }
    }
    if (!out) throw std::runtime_error("Failed while writing PPM output: " + path);
}
}

int main(int argc, char** argv) {
    try {
        if (argc < 3) { usage(); return 2; }
        const std::string command = argv[1];
        const std::string input_path = argv[2];
        auto tim2 = ps2asset::texture::Tim2File::parse(ps2asset::texture::read_binary_file(input_path));

        if (command == "info") {
            std::cout << "File: " << input_path << "\n"
                      << "Version: " << static_cast<unsigned int>(tim2.version()) << "\n"
                      << "Format type: " << static_cast<unsigned int>(tim2.format_type()) << "\n"
                      << "Pictures: " << tim2.picture_count() << "\n";
            for (std::size_t i = 0; i < tim2.pictures().size(); ++i) {
                const auto& p = tim2.pictures()[i];
                std::cout << "Picture " << i << ": " << p.width << 'x' << p.height
                          << ", " << p.image_type_name()
                          << ", CLUT type=" << static_cast<unsigned int>(p.clut_type)
                          << ", CLUT colors=" << p.clut_colors
                          << ", PSM=" << static_cast<unsigned int>(p.psm())
                          << ", header=" << p.header_size
                          << ", image bytes=" << p.image_size
                          << ", CLUT bytes=" << p.clut_size
                          << ", mipmaps=" << static_cast<unsigned int>(p.mipmap_count)
                          << ", import/replace=" << (p.supports_rgba8_palette_replace() ? "supported" : "unsupported")
                          << '\n';
            }
            return 0;
        }

        if (command == "export" || command == "export-png") {
            if (argc != 5) { usage(); return 2; }
            std::size_t index = 0;
            try {
                const auto parsed = std::stoull(argv[3]);
                if (parsed > static_cast<unsigned long long>(std::numeric_limits<std::size_t>::max())) {
                    throw std::out_of_range("picture index");
                }
                index = static_cast<std::size_t>(parsed);
            } catch (...) {
                throw std::runtime_error("Picture index must be a non-negative integer");
            }
            const auto image = tim2.decode_picture(index);
            if (command == "export") {
                write_ppm(argv[4], image);
            } else {
#ifdef PS2ASSET_HAS_PNG
                if (std::filesystem::exists(std::filesystem::path(argv[4]))) {
                    throw std::runtime_error("Output already exists; choose a new PNG path to avoid overwriting data");
                }
                ps2asset::image::write_png(argv[4], image);
#else
                throw std::runtime_error("PNG support is unavailable; rebuild with libpng development files");
#endif
            }
            std::cout << "Exported picture " << index << " to " << argv[4] << " (" << image.width << 'x' << image.height << ")\n";
            return 0;
        }

#ifdef PS2ASSET_HAS_PNG
        if (command == "replace-png") {
            if (argc != 6) { usage(); return 2; }
            std::size_t index = 0;
            try {
                const auto parsed = std::stoull(argv[3]);
                if (parsed > static_cast<unsigned long long>(std::numeric_limits<std::size_t>::max())) {
                    throw std::out_of_range("picture index");
                }
                index = static_cast<std::size_t>(parsed);
            } catch (...) {
                throw std::runtime_error("Picture index must be a non-negative integer");
            }
            const std::filesystem::path output_path(argv[5]);
            if (std::filesystem::exists(output_path)) {
                throw std::runtime_error("Output already exists; choose a new .tm2 path to avoid overwriting data");
            }
            auto replacement = ps2asset::image::read_png(argv[4]);
            tim2.replace_picture_rgba8_palette(index, replacement);
            const auto reparsed = ps2asset::texture::Tim2File::parse(tim2.bytes());
            const auto decoded = reparsed.decode_picture(index);
            if (!decoded.valid() || decoded.width != replacement.width || decoded.height != replacement.height) {
                throw std::runtime_error("Post-save validation failed: replaced texture did not decode to a valid image");
            }
            ps2asset::texture::write_binary_file_atomic(argv[5], tim2.bytes());
            std::cout << "Replaced picture " << index << " from " << argv[4] << " and wrote " << argv[5]
                      << " (" << tim2.bytes().size() << " bytes); indexed palette quantization and PS2 alpha precision apply; input was not overwritten\n";
            return 0;
        }
#endif

        usage();
        return 2;
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        return 1;
    }
}
