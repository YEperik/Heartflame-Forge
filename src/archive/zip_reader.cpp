#include "ps2asset/archive/zip_reader.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>

#ifdef PS2ASSET_HAS_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif

namespace ps2asset::archive {
namespace {
std::string lower_ascii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

bool has_tm2_extension(const std::string& path) {
    const auto lower = lower_ascii(path);
    return lower.size() >= 4U && lower.compare(lower.size() - 4U, 4U, ".tm2") == 0;
}
}  // namespace

bool zip_support_available() noexcept {
#ifdef PS2ASSET_HAS_LIBARCHIVE
    return true;
#else
    return false;
#endif
}

std::vector<ArchivedTexture> read_tim2_entries_from_zip(const std::string& archive_path,
                                                        std::uint64_t max_entry_size,
                                                        std::uint64_t max_total_texture_bytes) {
#ifndef PS2ASSET_HAS_LIBARCHIVE
    (void)archive_path;
    (void)max_entry_size;
    (void)max_total_texture_bytes;
    throw std::runtime_error("ZIP browsing is unavailable because this build was configured without libarchive");
#else
    struct ArchiveDeleter {
        void operator()(struct archive* value) const noexcept {
            if (value != nullptr) archive_read_free(value);
        }
    };
    std::unique_ptr<struct archive, ArchiveDeleter> reader(archive_read_new());
    if (!reader) throw std::runtime_error("libarchive could not allocate a ZIP reader");
    archive_read_support_filter_all(reader.get());
    archive_read_support_format_zip(reader.get());
    if (archive_read_open_filename(reader.get(), archive_path.c_str(), 64U * 1024U) != ARCHIVE_OK) {
        const char* error = archive_error_string(reader.get());
        throw std::runtime_error("Cannot open ZIP archive '" + archive_path + "': " + (error ? error : "unknown error"));
    }

    std::vector<ArchivedTexture> result;
    std::uint64_t total_bytes = 0;
    archive_entry* entry = nullptr;
    for (;;) {
        const int status = archive_read_next_header(reader.get(), &entry);
        if (status == ARCHIVE_EOF) break;
        if (status < ARCHIVE_WARN) {
            const char* error = archive_error_string(reader.get());
            throw std::runtime_error("Error while reading ZIP directory: " + std::string(error ? error : "unknown error"));
        }
        if (archive_entry_filetype(entry) != AE_IFREG) {
            archive_read_data_skip(reader.get());
            continue;
        }
        const char* raw_name = archive_entry_pathname_utf8(entry);
        if (raw_name == nullptr) raw_name = archive_entry_pathname(entry);
        const std::string entry_name = raw_name ? raw_name : "(unnamed entry)";
        if (!has_tm2_extension(entry_name)) {
            archive_read_data_skip(reader.get());
            continue;
        }

        const auto declared_size = archive_entry_size(entry);
        if (declared_size < 0 || static_cast<std::uint64_t>(declared_size) > max_entry_size) {
            throw std::runtime_error("Texture entry has an invalid or excessive uncompressed size: " + entry_name);
        }
        if (static_cast<std::uint64_t>(declared_size) > max_total_texture_bytes - total_bytes) {
            throw std::runtime_error("Uncompressed texture data exceeds the configured archive safety limit");
        }
        ArchivedTexture texture;
        texture.entry_name = entry_name;
        texture.bytes.reserve(static_cast<std::size_t>(declared_size));
        std::uint8_t buffer[64U * 1024U];
        for (;;) {
            const auto read_count = archive_read_data(reader.get(), buffer, sizeof(buffer));
            if (read_count == 0) break;
            if (read_count < 0) {
                const char* error = archive_error_string(reader.get());
                throw std::runtime_error("Cannot extract texture entry '" + entry_name + "': " + (error ? error : "unknown error"));
            }
            const auto count = static_cast<std::size_t>(read_count);
            if (count > max_entry_size - texture.bytes.size() ||
                count > max_total_texture_bytes - total_bytes - texture.bytes.size()) {
                throw std::runtime_error("Texture entry exceeds configured extraction limit: " + entry_name);
            }
            texture.bytes.insert(texture.bytes.end(), buffer, buffer + count);
        }
        total_bytes += texture.bytes.size();
        if (texture.bytes.size() != static_cast<std::size_t>(declared_size)) {
            throw std::runtime_error("Extracted texture size does not match ZIP metadata: " + entry_name);
        }
        result.push_back(std::move(texture));
        if (result.size() > 4096U) throw std::runtime_error("ZIP archive contains too many TIM2 textures (limit 4096)");
    }
    if (result.empty()) throw std::runtime_error("No .tm2 textures were found in the ZIP archive");
    return result;
#endif
}

}  // namespace ps2asset::archive
