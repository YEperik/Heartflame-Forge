#include "ps2asset/inspect.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace ps2asset {
namespace {

bool starts_with(const std::vector<std::uint8_t>& b,
                 const std::vector<std::uint8_t>& sig) {
    return b.size() >= sig.size() && std::equal(sig.begin(), sig.end(), b.begin());
}

std::uint16_t u16le(const std::vector<std::uint8_t>& b, std::size_t off) {
    return static_cast<std::uint16_t>(b[off]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(b[off + 1]) << 8U);
}

std::uint32_t u32le(const std::vector<std::uint8_t>& b, std::size_t off) {
    return static_cast<std::uint32_t>(b[off]) |
           (static_cast<std::uint32_t>(b[off + 1]) << 8U) |
           (static_cast<std::uint32_t>(b[off + 2]) << 16U) |
           (static_cast<std::uint32_t>(b[off + 3]) << 24U);
}

bool contains_ascii(const std::vector<std::uint8_t>& b, const std::string& s,
                    std::size_t* at = nullptr) {
    const auto it = std::search(b.begin(), b.end(), s.begin(), s.end());
    if (it == b.end()) return false;
    if (at) *at = static_cast<std::size_t>(std::distance(b.begin(), it));
    return true;
}

}  // namespace

std::string hex_u32(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setw(8)
        << std::setfill('0') << value;
    return out.str();
}

InspectionResult inspect_header(const std::vector<std::uint8_t>& b,
                                std::uint64_t full_size) {
    InspectionResult r;
    r.format = "unknown";

    if (starts_with(b, {'S', 'L', '0', '1'})) {
        r.format = "SL01 container/resource (structure partial)";
        r.is_sl01 = true;
        if (b.size() < 12 || full_size < 12) {
            r.details.emplace_back("truncated: fixed 12-byte prefix unavailable");
            return r;
        }
        const std::uint32_t field04 = u32le(b, 4);
        const std::uint32_t field08 = u32le(b, 8);
        r.details.emplace_back("field@0x04=" + hex_u32(field04) + " (semantics unknown)");
        r.details.emplace_back("field@0x08=" + std::to_string(field08));
        r.details.emplace_back("actual bytes after 0x0C=" + std::to_string(full_size - 12));
        r.sl01_length_checked = true;
        r.sl01_length_matches = (static_cast<std::uint64_t>(field08) == full_size - 12);
        r.details.emplace_back(std::string("field@0x08 vs size-12: ") +
                               (r.sl01_length_matches ? "MATCH" : "MISMATCH"));
        return r;
    }

    if (starts_with(b, {'T', 'I', 'M', '2'})) {
        r.format = "TIM2 texture";
        if (b.size() >= 8) {
            r.details.emplace_back("version=" + std::to_string(u16le(b, 4)));
            r.details.emplace_back("picture_count=" + std::to_string(u16le(b, 6)));
        } else {
            r.details.emplace_back("truncated: TIM2 header shorter than 8 bytes");
        }
        return r;
    }

    if (starts_with(b, {0x00, 0x00, 0x01, 0xBA})) {
        r.format = "MPEG program stream (PSS-like video)";
        r.details.emplace_back("pack start code 00 00 01 BA");
        return r;
    }

    if (starts_with(b, {0x7F, 'E', 'L', 'F'})) {
        r.format = "ELF executable/module";
        if (b.size() >= 20) {
            r.details.emplace_back("class=" + std::to_string(b[4]) +
                                   " (1 means ELF32)");
            r.details.emplace_back("data_encoding=" + std::to_string(b[5]) +
                                   " (1 means little-endian)");
            r.details.emplace_back("e_type=" + hex_u32(u16le(b, 16)));
            r.details.emplace_back("e_machine=" + hex_u32(u16le(b, 18)));
        }
        return r;
    }

    if (starts_with(b, {'M', 'I', 'G', '.', '0', '0', '.', '1', 'P', 'S', 'P'})) {
        r.format = "Sony GIM image";
        r.details.emplace_back("signature MIG.00.1PSP");
        return r;
    }

    if (starts_with(b, {'S', 'D', 'P', 'K'})) {
        r.format = "SDPK audio/resource (internals not decoded)";
        r.details.emplace_back("signature SDPK");
        return r;
    }

    if (starts_with(b, {'A', 'D', 'S', 'C'})) {
        r.format = "ADSC audio/resource (internals not decoded)";
        r.details.emplace_back("signature ADSC");
        if (b.size() >= 12) {
            r.details.emplace_back("next chunk tag=" +
                                   std::string(1, static_cast<char>(b[8])) +
                                   std::string(1, static_cast<char>(b[9])) +
                                   std::string(1, static_cast<char>(b[10])) +
                                   std::string(1, static_cast<char>(b[11])));
        }
        return r;
    }

    if (starts_with(b, {'.', 'G', 'I', 'S'})) {
        r.format = "GIS text image description";
        r.details.emplace_back("text signature .GIS");
        return r;
    }

    std::size_t romdir_at = 0;
    if (contains_ascii(b, "ROMDIR", &romdir_at)) {
        r.format = "PS2 IOPRP-like image (ROMDIR marker found)";
        r.details.emplace_back("ROMDIR marker at prefix offset " + std::to_string(romdir_at));
        return r;
    }

    if (!b.empty()) {
        std::ostringstream out;
        out << "prefix=" << std::uppercase << std::hex << std::setfill('0');
        const auto n = std::min<std::size_t>(b.size(), 16);
        for (std::size_t i = 0; i < n; ++i) {
            if (i) out << ' ';
            out << std::setw(2) << static_cast<unsigned int>(b[i]);
        }
        r.details.emplace_back(out.str());
    } else {
        r.details.emplace_back("empty file or empty prefix");
    }
    return r;
}

}  // namespace ps2asset
