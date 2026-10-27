#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ps2asset::text {

enum class StringEncoding {
    ASCII,
    UTF16LE_ASCII_SUBSET
};

struct StringHit {
    std::uint64_t offset = 0;
    StringEncoding encoding = StringEncoding::ASCII;
    std::string value;
};

struct StringScanResult {
    std::vector<StringHit> hits;
    bool truncated_by_limit = false;
    std::uint64_t bytes_scanned = 0;
};

// Finds printable ASCII runs and UTF-16LE runs whose codepoints are printable
// ASCII. It is a discovery aid, not a script/locale decoder.
StringScanResult scan_printable_strings(const std::vector<std::uint8_t>& bytes,
                                        std::size_t minimum_length = 4U,
                                        std::size_t maximum_results = 10000U);
std::string encoding_name(StringEncoding encoding);
std::string format_string_scan_report(const StringScanResult& result);

}  // namespace ps2asset::text
