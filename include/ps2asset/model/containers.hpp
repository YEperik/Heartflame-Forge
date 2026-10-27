#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ps2asset::model {

// A validated SL01 byte range. total_size includes the 12-byte SL01 header.
// The stored field at +0x08 is treated only as the empirically observed
// byte count after the header; no other field semantics are inferred.
struct EmbeddedSl01Block {
    std::uint64_t offset = 0;
    std::uint32_t size_field = 0;
    std::uint64_t total_size = 0;
    std::uint64_t gap_after = 0;
};

struct EmbeddedSl01ScanResult {
    std::uint64_t source_size = 0;
    std::uint64_t signature_hits = 0;
    std::uint64_t rejected_candidates = 0;
    bool scan_aborted_hit_limit = false;
    std::vector<EmbeddedSl01Block> blocks;
    std::vector<std::string> warnings;
};

// Finds SL01 blocks whose stored length fits in the input. A candidate must
// not overlap the next SL01 hit, except when a block at offset 0 describes the
// entire file; in that case internal SL01 byte patterns are treated as payload.
// The function is read-only and never writes files.
EmbeddedSl01ScanResult scan_embedded_sl01_blocks(
    const std::vector<std::uint8_t>& bytes);

}  // namespace ps2asset::model
