#include "ps2asset/archive/zip_reader.hpp"
#include "ps2asset/io/new_file_transaction.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <memory>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#ifdef PS2ASSET_HAS_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif

namespace ps2asset::archive {
#ifdef PS2ASSET_HAS_LIBARCHIVE
namespace {
constexpr std::uint64_t kMaxArchiveEntries = 200000ULL;
constexpr std::size_t kMaxReplacementTargets = 4096U;

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
#endif

std::vector<ArchiveEntryInfo> list_zip_entries(const std::string& archive_path,
                                                std::uint64_t max_entries) {
#ifndef PS2ASSET_HAS_LIBARCHIVE
    (void)archive_path;
    (void)max_entries;
    throw std::runtime_error("ZIP browsing is unavailable because this build was configured without libarchive");
#else
    if (max_entries == 0U) throw std::runtime_error("ZIP entry-list limit must be greater than zero");
    struct ArchiveDeleter {
        void operator()(struct archive* value) const noexcept {
            if (value != nullptr) archive_read_free(value);
        }
    };
    std::unique_ptr<struct archive, ArchiveDeleter> reader(archive_read_new());
    if (!reader) throw std::runtime_error("libarchive could not allocate an archive-list reader");
    archive_read_support_filter_all(reader.get());
    archive_read_support_format_zip(reader.get());
    if (archive_read_open_filename(reader.get(), archive_path.c_str(), 64U * 1024U) != ARCHIVE_OK) {
        const char* error = archive_error_string(reader.get());
        throw std::runtime_error("Cannot open ZIP archive '" + archive_path + "': " + (error ? error : "unknown error"));
    }

    std::vector<ArchiveEntryInfo> result;
    archive_entry* entry = nullptr;
    for (;;) {
        const int status = archive_read_next_header(reader.get(), &entry);
        if (status == ARCHIVE_EOF) break;
        if (status < ARCHIVE_WARN) {
            const char* error = archive_error_string(reader.get());
            throw std::runtime_error("Error while reading ZIP directory: " + std::string(error ? error : "unknown error"));
        }
        if (result.size() >= max_entries) {
            throw std::runtime_error("ZIP archive has more entries than the configured listing limit");
        }
        const char* raw_name = archive_entry_pathname_utf8(entry);
        if (raw_name == nullptr) raw_name = archive_entry_pathname(entry);
        const auto file_type = archive_entry_filetype(entry);
        const auto declared_size = archive_entry_size(entry);
        ArchiveEntryInfo info;
        info.path = raw_name != nullptr ? raw_name : "(unnamed entry)";
        info.is_directory = file_type == AE_IFDIR;
        info.is_regular_file = file_type == AE_IFREG;
        if (declared_size >= 0) info.uncompressed_size = static_cast<std::uint64_t>(declared_size);
        result.push_back(std::move(info));
        archive_read_data_skip(reader.get());
    }
    return result;
#endif
}


void replace_zip_entry(const std::string& archive_path, const std::string& entry_path,
                       const std::string& replacement_file, const std::string& output_archive,
                       std::uint64_t max_replacement_size) {
#ifndef PS2ASSET_HAS_LIBARCHIVE
    (void)archive_path; (void)entry_path; (void)replacement_file; (void)output_archive; (void)max_replacement_size;
    throw std::runtime_error("ZIP writing is unavailable because this build was configured without libarchive");
#else
    namespace fs = std::filesystem;
    std::error_code ec;
    const auto replacement_size = fs::file_size(replacement_file, ec);
    if (ec || replacement_size > max_replacement_size) throw std::runtime_error("Replacement file is missing or exceeds size limit");
    if (replacement_size > std::numeric_limits<std::size_t>::max() ||
        replacement_size > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
        throw std::runtime_error("Replacement file is too large for this platform or stream API");
    }
    std::vector<std::uint8_t> replacement(static_cast<std::size_t>(replacement_size));
    std::ifstream replacement_stream(replacement_file, std::ios::binary);
    if (!replacement_stream || (!replacement.empty() && !replacement_stream.read(reinterpret_cast<char*>(replacement.data()), static_cast<std::streamsize>(replacement.size()))))
        throw std::runtime_error("Unable to read replacement file");
    replace_zip_entry_bytes(archive_path, entry_path, replacement, output_archive, max_replacement_size);
#endif
}

void replace_zip_entry_bytes(const std::string& archive_path, const std::string& entry_path,
                             const std::vector<std::uint8_t>& replacement, const std::string& output_archive,
                             std::uint64_t max_replacement_size) {
    ZipEntryReplacement item;
    item.entry_path = entry_path;
    item.bytes = replacement;
    replace_zip_entries_bytes(archive_path, {item}, output_archive, max_replacement_size,
                              max_replacement_size);
}

void replace_zip_entries_bytes(const std::string& archive_path,
                               const std::vector<ZipEntryReplacement>& replacements,
                               const std::string& output_archive,
                               std::uint64_t max_replacement_size,
                               std::uint64_t max_total_replacement_size) {
#ifndef PS2ASSET_HAS_LIBARCHIVE
    (void)archive_path; (void)replacements; (void)output_archive;
    (void)max_replacement_size; (void)max_total_replacement_size;
    throw std::runtime_error("ZIP writing is unavailable because this build was configured without libarchive");
#else
    namespace fs = std::filesystem;
    if (replacements.empty()) throw std::runtime_error("At least one ZIP entry replacement is required");
    if (max_replacement_size == 0U || max_total_replacement_size == 0U)
        throw std::runtime_error("Replacement size limits must be positive");
    if (replacements.size() > kMaxReplacementTargets)
        throw std::runtime_error("Too many replacement targets (limit 4096)");

    std::unordered_map<std::string, std::size_t> replacement_by_path;
    replacement_by_path.reserve(replacements.size());
    std::uint64_t total_replacement_bytes = 0;
    for (std::size_t i = 0; i < replacements.size(); ++i) {
        const auto& replacement = replacements[i];
        if (replacement.entry_path.empty()) throw std::runtime_error("ZIP entry path must not be empty");
        if (static_cast<std::uint64_t>(replacement.bytes.size()) > max_replacement_size)
            throw std::runtime_error("Replacement data exceeds per-entry size limit: " + replacement.entry_path);
        const auto size = static_cast<std::uint64_t>(replacement.bytes.size());
        if (size > max_total_replacement_size - total_replacement_bytes)
            throw std::runtime_error("Combined replacement data exceeds configured size limit");
        total_replacement_bytes += size;
        if (!replacement_by_path.emplace(replacement.entry_path, i).second)
            throw std::runtime_error("Duplicate replacement target path: " + replacement.entry_path);
    }

    struct ReadDeleter { void operator()(struct archive* p) const noexcept { if (p) archive_read_free(p); } };
    struct WriteDeleter { void operator()(struct archive* p) const noexcept { if (p) archive_write_free(p); } };
    std::unique_ptr<struct archive, ReadDeleter> reader(archive_read_new());
    std::unique_ptr<struct archive, WriteDeleter> writer(archive_write_new());
    if (!reader || !writer) throw std::runtime_error("Unable to allocate ZIP reader/writer");
    archive_read_support_filter_all(reader.get());
    archive_read_support_format_zip(reader.get());
    if (archive_read_open_filename(reader.get(), archive_path.c_str(), 64U * 1024U) != ARCHIVE_OK)
        throw std::runtime_error(std::string("Unable to open source ZIP: ") + (archive_error_string(reader.get()) ? archive_error_string(reader.get()) : "unknown error"));
    if (archive_write_set_format_zip(writer.get()) != ARCHIVE_OK)
        throw std::runtime_error("Unable to configure ZIP writer");

    ps2asset::io::NewFileTransaction output_transaction{fs::path(output_archive)};
    const fs::path& temp_path = output_transaction.temporary_path();
    std::unordered_set<std::string> found_targets;
    found_targets.reserve(replacements.size());
    std::uint64_t entries_seen = 0U;
    try {
        if (archive_write_open_filename(writer.get(), temp_path.string().c_str()) != ARCHIVE_OK)
            throw std::runtime_error(std::string("Unable to create temporary ZIP: ") + (archive_error_string(writer.get()) ? archive_error_string(writer.get()) : "unknown error"));
        archive_entry* input_entry = nullptr;
        std::vector<std::uint8_t> buffer(128U * 1024U);
        for (;;) {
            const int status = archive_read_next_header(reader.get(), &input_entry);
            if (status == ARCHIVE_EOF) break;
            if (status < ARCHIVE_WARN) throw std::runtime_error(std::string("Error reading source ZIP: ") + (archive_error_string(reader.get()) ? archive_error_string(reader.get()) : "unknown error"));
            if (++entries_seen > kMaxArchiveEntries)
                throw std::runtime_error("Source ZIP exceeds the 200000-entry safety limit");
            const char* raw_name = archive_entry_pathname_utf8(input_entry);
            if (!raw_name) raw_name = archive_entry_pathname(input_entry);
            const std::string name = raw_name ? raw_name : "";
            const auto replacement_it = replacement_by_path.find(name);
            const bool target = replacement_it != replacement_by_path.end();
            if (target) {
                if (!found_targets.insert(name).second || archive_entry_filetype(input_entry) != AE_IFREG)
                    throw std::runtime_error("Target ZIP entry is duplicated or is not a regular file: " + name);
            }
            archive_entry* output_entry = archive_entry_clone(input_entry);
            if (!output_entry) throw std::runtime_error("Unable to clone ZIP entry metadata");
            if (target) archive_entry_set_size(output_entry, static_cast<la_int64_t>(replacements[replacement_it->second].bytes.size()));
            if (archive_write_header(writer.get(), output_entry) != ARCHIVE_OK) {
                const std::string error = archive_error_string(writer.get()) ? archive_error_string(writer.get()) : "unknown error";
                archive_entry_free(output_entry);
                throw std::runtime_error("Unable to write ZIP entry header: " + error);
            }
            archive_entry_free(output_entry);
            if (target) {
                const auto& bytes = replacements[replacement_it->second].bytes;
                std::size_t offset = 0;
                while (offset < bytes.size()) {
                    const auto n = archive_write_data(writer.get(), bytes.data() + offset, bytes.size() - offset);
                    if (n <= 0) throw std::runtime_error("Unable to write replacement entry payload: " + name);
                    offset += static_cast<std::size_t>(n);
                }
                archive_read_data_skip(reader.get());
            } else {
                for (;;) {
                    const auto n = archive_read_data(reader.get(), buffer.data(), buffer.size());
                    if (n == 0) break;
                    if (n < 0) throw std::runtime_error(std::string("Unable to read ZIP entry payload: ") + (archive_error_string(reader.get()) ? archive_error_string(reader.get()) : "unknown error"));
                    std::size_t offset = 0;
                    const auto count = static_cast<std::size_t>(n);
                    while (offset < count) {
                        const auto written = archive_write_data(writer.get(), buffer.data() + offset, count - offset);
                        if (written <= 0) throw std::runtime_error("Unable to copy ZIP entry payload");
                        offset += static_cast<std::size_t>(written);
                    }
                }
            }
        }
        if (found_targets.size() != replacements.size()) {
            for (const auto& replacement : replacements) {
                if (found_targets.find(replacement.entry_path) == found_targets.end())
                    throw std::runtime_error("Target entry was not found in source ZIP: " + replacement.entry_path);
            }
        }
        if (archive_write_close(writer.get()) != ARCHIVE_OK) throw std::runtime_error(std::string("Unable to finalize ZIP: ") + (archive_error_string(writer.get()) ? archive_error_string(writer.get()) : "unknown error"));
        if (archive_read_close(reader.get()) != ARCHIVE_OK) throw std::runtime_error("Unable to close source ZIP cleanly");
        output_transaction.commit();
    } catch (...) {
        // Release archive handles before the transaction removes its private
        // temporary directory (important on Windows).
        writer.reset();
        reader.reset();
        throw;
    }
#endif
}

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
    std::uint64_t entries_seen = 0U;
    archive_entry* entry = nullptr;
    for (;;) {
        const int status = archive_read_next_header(reader.get(), &entry);
        if (status == ARCHIVE_EOF) break;
        if (status < ARCHIVE_WARN) {
            const char* error = archive_error_string(reader.get());
            throw std::runtime_error("Error while reading ZIP directory: " + std::string(error ? error : "unknown error"));
        }
        if (++entries_seen > 200000ULL) throw std::runtime_error("ZIP exceeds the 200000-entry safety limit");
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
