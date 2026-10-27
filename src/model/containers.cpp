#include "ps2asset/model/containers.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <string_view>

namespace ps2asset::model {
namespace {

std::uint32_t read_u32_le(const std::vector<std::uint8_t>& bytes, std::size_t off) {
    return static_cast<std::uint32_t>(bytes[off]) |
           (static_cast<std::uint32_t>(bytes[off + 1U]) << 8U) |
           (static_cast<std::uint32_t>(bytes[off + 2U]) << 16U) |
           (static_cast<std::uint32_t>(bytes[off + 3U]) << 24U);
}

struct HitCollection {
    std::vector<std::uint64_t> offsets;
    bool limit_exceeded = false;
};

HitCollection find_hits(const std::vector<std::uint8_t>& bytes) {
    constexpr std::array<std::uint8_t, 4> sig{{'S', 'L', '0', '1'}};
    constexpr std::size_t kMaxSignatureHits = 100000U;
    HitCollection result;
    if (bytes.size() < sig.size()) return result;
    result.offsets.reserve(std::min<std::size_t>(bytes.size() / 1024U, 4096U));
    auto cursor = bytes.begin();
    while (cursor != bytes.end()) {
        const auto found = std::search(cursor, bytes.end(), sig.begin(), sig.end());
        if (found == bytes.end()) break;
        if (result.offsets.size() >= kMaxSignatureHits) {
            result.limit_exceeded = true;
            break;
        }
        result.offsets.push_back(static_cast<std::uint64_t>(std::distance(bytes.begin(), found)));
        cursor = found + 1;
    }
    return result;
}

void warning_once(EmbeddedSl01ScanResult& result, const std::string& message) {
    constexpr std::size_t kMaxWarnings = 16U;
    if (result.warnings.size() < kMaxWarnings) result.warnings.push_back(message);
}

}  // namespace

EmbeddedSl01ScanResult scan_embedded_sl01_blocks(
    const std::vector<std::uint8_t>& bytes) {
    EmbeddedSl01ScanResult result;
    result.source_size = static_cast<std::uint64_t>(bytes.size());
    const auto hit_collection = find_hits(bytes);
    const auto& hits = hit_collection.offsets;
    result.signature_hits = static_cast<std::uint64_t>(hits.size());
    if (hit_collection.limit_exceeded) {
        result.scan_aborted_hit_limit = true;
        warning_once(result, "More than 100000 SL01 signatures were encountered; scanning stopped to keep memory bounded.");
        return result;
    }

    std::uint64_t cursor = 0;
    while (cursor < result.source_size) {
        const auto hit_it = std::lower_bound(hits.begin(), hits.end(), cursor);
        if (hit_it == hits.end()) break;
        const std::uint64_t offset = *hit_it;

        const std::uint64_t remaining = result.source_size - offset;
        if (remaining < 12U) {
            ++result.rejected_candidates;
            warning_once(result, "SL01 signature at offset " + std::to_string(offset) +
                                 " has a truncated 12-byte header.");
            cursor = offset + 4U;
            continue;
        }

        const auto offset_size = static_cast<std::size_t>(offset);
        const std::uint32_t field = read_u32_le(bytes, offset_size + 8U);
        const std::uint64_t total_size = static_cast<std::uint64_t>(field) + 12U;
        if (total_size > remaining) {
            ++result.rejected_candidates;
            warning_once(result, "SL01 candidate at offset " + std::to_string(offset) +
                                 " declares bytes beyond the input range.");
            cursor = offset + 4U;
            continue;
        }

        const std::uint64_t end = offset + total_size;
        const auto next_hit_it = std::upper_bound(hits.begin(), hits.end(), offset);
        const bool describes_whole_file = (offset == 0U && end == result.source_size);
        if (!describes_whole_file && next_hit_it != hits.end() && *next_hit_it < end) {
            ++result.rejected_candidates;
            warning_once(result, "SL01 candidate at offset " + std::to_string(offset) +
                                 " overlaps another SL01 signature; range left undecoded.");
            cursor = offset + 4U;
            continue;
        }

        EmbeddedSl01Block block;
        block.offset = offset;
        block.size_field = field;
        block.total_size = total_size;
        result.blocks.push_back(block);

        // If an outer file is entirely described by its leading wrapper, any
        // SL01-looking bytes inside it are payload patterns, not new blocks.
        if (describes_whole_file) {
            cursor = result.source_size;
            break;
        }
        // Jump past the declared block; any SL01 signatures inside its body
        // were not used as additional records.
        cursor = end;
    }

    for (std::size_t i = 0; i < result.blocks.size(); ++i) {
        const std::uint64_t current_end =
            result.blocks[i].offset + result.blocks[i].total_size;
        const std::uint64_t next_start = (i + 1U < result.blocks.size())
            ? result.blocks[i + 1U].offset
            : result.source_size;
        if (next_start >= current_end) {
            result.blocks[i].gap_after = next_start - current_end;
        }
    }

    if (result.blocks.empty() && result.signature_hits > 0U) {
        warning_once(result, "SL01 byte patterns were found, but no candidate range passed the bounds/overlap checks.");
    } else if (result.blocks.empty()) {
        warning_once(result, "No SL01 signatures were found in this input.");
    }

    return result;
}

}  // namespace ps2asset::model
