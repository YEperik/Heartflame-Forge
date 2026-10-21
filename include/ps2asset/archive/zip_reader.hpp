#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ps2asset::archive {

struct ArchivedTexture {
    std::string entry_name;
    std::vector<std::uint8_t> bytes;
};

struct ArchiveEntryInfo {
    std::string path;
    std::uint64_t uncompressed_size = 0;
    bool is_directory = false;
    bool is_regular_file = false;
};

// One entry to replace while rebuilding an archive. The entry path must match
// the ZIP pathname exactly, and the byte payload is copied into the new ZIP.
struct ZipEntryReplacement {
    std::string entry_path;
    std::vector<std::uint8_t> bytes;
};

// Lists archive metadata only. No entry payload is extracted or modified.
[[nodiscard]] std::vector<ArchiveEntryInfo> list_zip_entries(
    const std::string& archive_path,
    std::uint64_t max_entries = 200000ULL);

// Rebuilds a ZIP with exactly one regular-file entry replaced. Metadata for all
// other entries is copied; output must not already exist. A temporary sibling
// is used so failed writes do not leave a partial output at the requested path.
void replace_zip_entry(const std::string& archive_path,
                       const std::string& entry_path,
                       const std::string& replacement_file,
                       const std::string& output_archive,
                       std::uint64_t max_replacement_size = 32ULL * 1024ULL * 1024ULL);

// Replaces multiple regular-file entries in a single rebuild. Entry paths must
// be unique and all requested targets must exist exactly once in the source ZIP.
// Each payload is capped at max_replacement_size and the combined payload is
// capped at max_total_replacement_size to bound memory use.
void replace_zip_entries_bytes(const std::string& archive_path,
                               const std::vector<ZipEntryReplacement>& replacements,
                               const std::string& output_archive,
                               std::uint64_t max_replacement_size = 32ULL * 1024ULL * 1024ULL,
                               std::uint64_t max_total_replacement_size = 256ULL * 1024ULL * 1024ULL);

// In-memory variant for a single target; delegates to the batch API.
void replace_zip_entry_bytes(const std::string& archive_path,
                             const std::string& entry_path,
                             const std::vector<std::uint8_t>& replacement,
                             const std::string& output_archive,
                             std::uint64_t max_replacement_size = 32ULL * 1024ULL * 1024ULL);

[[nodiscard]] bool zip_support_available() noexcept;
[[nodiscard]] std::vector<ArchivedTexture> read_tim2_entries_from_zip(
    const std::string& archive_path,
    std::uint64_t max_entry_size = 32ULL * 1024ULL * 1024ULL,
    std::uint64_t max_total_texture_bytes = 256ULL * 1024ULL * 1024ULL);

}  // namespace ps2asset::archive
