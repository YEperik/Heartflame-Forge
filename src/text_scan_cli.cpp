#include "ps2asset/text/scan.hpp"
#include "ps2asset/io/new_file_transaction.hpp"

#include <algorithm>
#include <charconv>
#include <system_error>
#include <stdexcept>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace {
constexpr std::uintmax_t kMaxInputBytes = 64U * 1024U * 1024U;
constexpr std::size_t kDefaultMinLength = 4U;
constexpr std::size_t kDefaultMaxResults = 10000U;

bool parse_size(const std::string& s, std::size_t& value) {
    if (s.empty() || !std::all_of(s.begin(), s.end(), [](unsigned char c) { return c >= '0' && c <= '9'; })) {
        return false;
    }
    unsigned long long parsed = 0U;
    const auto result = std::from_chars(s.data(), s.data() + s.size(), parsed, 10);
    if (result.ec != std::errc{} || result.ptr != s.data() + s.size() ||
        parsed > static_cast<unsigned long long>(std::numeric_limits<std::size_t>::max())) {
        return false;
    }
    value = static_cast<std::size_t>(parsed);
    return true;
}

std::string tsv_escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char c : value) {
        switch (c) {
        case '\\': escaped += "\\\\"; break;
        case '\t': escaped += "\\t"; break;
        case '\r': escaped += "\\r"; break;
        case '\n': escaped += "\\n"; break;
        default: escaped.push_back(c); break;
        }
    }
    return escaped;
}

bool write_report(std::ostream& out, const ps2asset::text::StringScanResult& result) {
    out << "offset_hex\toffset_decimal\tencoding\tvalue_escaped\n";
    for (const auto& hit : result.hits) {
        out << "0x" << std::hex << hit.offset << std::dec << '\t' << hit.offset << '\t'
            << ps2asset::text::encoding_name(hit.encoding) << '\t' << tsv_escape(hit.value) << '\n';
    }
    out << "# bytes_scanned=" << result.bytes_scanned << '\n';
    out << "# hit_count=" << result.hits.size() << '\n';
    out << "# truncated_by_limit=" << (result.truncated_by_limit ? "true" : "false") << '\n';
    return static_cast<bool>(out);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        std::cout << "Usage: ps2asset-text-scan <input.bin> [output.tsv] [--min-length N] [--max-results N]\n"
                     "Read-only scan for printable ASCII and UTF-16LE ASCII-subset strings.\n"
                     "Input limit: 64 MiB; default results limit: 10000. Existing output is never overwritten.\n";
        return argc < 2 ? 2 : 0;
    }

    fs::path inputPath(argv[1]);
    fs::path outputPath;
    bool hasOutput = false;
    std::size_t minimumLength = kDefaultMinLength;
    std::size_t maximumResults = kDefaultMaxResults;
    for (int i = 2; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--min-length" || arg == "--max-results") {
            if (i + 1 >= argc) {
                std::cerr << "ERROR: missing value for " << arg << '\n';
                return 2;
            }
            std::size_t parsed = 0;
            if (!parse_size(argv[++i], parsed)) {
                std::cerr << "ERROR: invalid numeric value for " << arg << '\n';
                return 2;
            }
            if (arg == "--min-length") minimumLength = parsed;
            else maximumResults = parsed;
        } else if (!hasOutput) {
            outputPath = fs::path(arg);
            hasOutput = true;
        } else {
            std::cerr << "ERROR: unexpected argument: " << arg << '\n';
            return 2;
        }
    }
    if (minimumLength < 2U || maximumResults == 0U || maximumResults > 100000U) {
        std::cerr << "ERROR: --min-length must be >= 2 and --max-results must be in range 1..100000\n";
        return 2;
    }

    std::error_code ec;
    if (!fs::is_regular_file(inputPath, ec) || ec) {
        std::cerr << "ERROR: input is not a regular file\n";
        return 1;
    }
    const auto size = fs::file_size(inputPath, ec);
    if (ec || size > kMaxInputBytes) {
        std::cerr << "ERROR: input size cannot be read or exceeds 64 MiB\n";
        return 1;
    }
    if (hasOutput && fs::exists(outputPath, ec)) {
        std::cerr << "ERROR: output already exists; refusing to overwrite\n";
        return 1;
    }

    std::ifstream in(inputPath, std::ios::binary);
    if (!in) {
        std::cerr << "ERROR: unable to open input\n";
        return 1;
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    if (!bytes.empty()) {
        in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (in.gcount() != static_cast<std::streamsize>(bytes.size())) {
            std::cerr << "ERROR: short read\n";
            return 1;
        }
    }

    ps2asset::text::StringScanResult result;
    try {
        result = ps2asset::text::scan_printable_strings(bytes, minimumLength, maximumResults);
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << '\n';
        return 2;
    }

    if (!hasOutput) {
        if (!write_report(std::cout, result)) return 1;
    } else {
        try {
            ps2asset::io::write_new_file_atomically(outputPath, [&result](std::ostream& out) {
                if (!write_report(out, result)) {
                    throw std::runtime_error("failed to write TSV report");
                }
            });
        } catch (const std::exception& ex) {
            std::cerr << "ERROR: unable to create TSV report without overwriting existing files: "
                      << ex.what() << '\n';
            return 1;
        }
    }


    std::cerr << "SUMMARY hits=" << result.hits.size()
              << " truncated=" << (result.truncated_by_limit ? "yes" : "no") << '\n';
    return 0;
}
