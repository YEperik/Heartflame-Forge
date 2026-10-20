#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ps2asset {

struct InspectionResult {
    std::string format;
    std::vector<std::string> details;
    bool is_sl01 = false;
    bool sl01_length_checked = false;
    bool sl01_length_matches = false;
};

// Inspects a prefix read from a file. full_size must be the actual file size.
// The function never reads or writes asset payloads and makes no assumptions
// about fields whose semantics have not been established.
InspectionResult inspect_header(const std::vector<std::uint8_t>& prefix,
                                std::uint64_t full_size);

std::string hex_u32(std::uint32_t value);

}  // namespace ps2asset
