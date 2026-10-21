#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ps2asset::archive {

struct ArchivedTexture {
    std::string entry_name;
    std::vector<std::uint8_t> bytes;
};

[[nodiscard]] bool zip_support_available() noexcept;
[[nodiscard]] std::vector<ArchivedTexture> read_tim2_entries_from_zip(
    const std::string& archive_path,
    std::uint64_t max_entry_size = 32ULL * 1024ULL * 1024ULL,
    std::uint64_t max_total_texture_bytes = 256ULL * 1024ULL * 1024ULL);

}  // namespace ps2asset::archive
