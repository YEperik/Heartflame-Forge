#include "ps2asset/text/scan.hpp"

#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <tuple>

namespace ps2asset::text {
namespace {

bool printable_ascii(std::uint8_t c) {
    return c >= 0x20U && c <= 0x7EU;
}

}  // namespace

StringScanResult scan_printable_strings(const std::vector<std::uint8_t>& bytes,
                                        std::size_t minimum_length,
                                        std::size_t maximum_results) {
    if (minimum_length < 2U) throw std::invalid_argument("minimum_length must be >= 2");
    if (maximum_results == 0U) throw std::invalid_argument("maximum_results must be > 0");

    StringScanResult result;
    result.bytes_scanned = static_cast<std::uint64_t>(bytes.size());
    const auto append = [&result, maximum_results](StringHit hit) {
        if (result.hits.size() >= maximum_results) {
            result.truncated_by_limit = true;
            return false;
        }
        result.hits.push_back(std::move(hit));
        return true;
    };

    std::size_t i = 0U;
    while (i < bytes.size()) {
        if (!printable_ascii(bytes[i])) { ++i; continue; }
        const std::size_t begin = i;
        while (i < bytes.size() && printable_ascii(bytes[i])) ++i;
        if (i - begin >= minimum_length) {
            std::string value;
            value.reserve(i - begin);
            for (std::size_t k = begin; k < i; ++k) value.push_back(static_cast<char>(bytes[k]));
            if (!append(StringHit{static_cast<std::uint64_t>(begin), StringEncoding::ASCII, std::move(value)})) {
                return result;
            }
        }
    }

    i = 0U;
    while (i + 1U < bytes.size()) {
        if (!printable_ascii(bytes[i]) || bytes[i + 1U] != 0U) { ++i; continue; }
        const std::size_t begin = i;
        std::string value;
        while (i + 1U < bytes.size() && printable_ascii(bytes[i]) && bytes[i + 1U] == 0U) {
            value.push_back(static_cast<char>(bytes[i]));
            i += 2U;
        }
        if (value.size() >= minimum_length) {
            if (!append(StringHit{static_cast<std::uint64_t>(begin), StringEncoding::UTF16LE_ASCII_SUBSET,
                                  std::move(value)})) {
                result.truncated_by_limit = true;
                break;
            }
        }
    }

    std::stable_sort(result.hits.begin(), result.hits.end(), [](const StringHit& a, const StringHit& b) {
        return std::tie(a.offset, a.encoding) < std::tie(b.offset, b.encoding);
    });
    return result;
}

std::string encoding_name(StringEncoding encoding) {
    return encoding == StringEncoding::ASCII ? "ASCII" : "UTF-16LE (ASCII subset)";
}

std::string format_string_scan_report(const StringScanResult& result) {
    std::ostringstream out;
    out << "Printable string scan (read-only)\n"
        << "bytes scanned: " << result.bytes_scanned << '\n'
        << "hits: " << result.hits.size() << '\n'
        << "limit reached: " << (result.truncated_by_limit ? "yes" : "no") << '\n';
    for (const auto& hit : result.hits) {
        out << "0x" << std::hex << hit.offset << std::dec << '\t'
            << encoding_name(hit.encoding) << '\t' << hit.value << '\n';
    }
    out << "note: UTF-16 support is limited to strings with printable ASCII codepoints; format/encoding semantics are not inferred\n";
    return out.str();
}

}  // namespace ps2asset::text
